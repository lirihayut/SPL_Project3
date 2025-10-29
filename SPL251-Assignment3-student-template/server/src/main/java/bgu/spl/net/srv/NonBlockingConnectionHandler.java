package bgu.spl.net.srv;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SelectionKey;
import java.nio.channels.SocketChannel;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.MessageEncoderDecoder;

public class NonBlockingConnectionHandler<T> implements ConnectionHandler<T> {

    private static final int BUFFER_ALLOCATION_SIZE = 1 << 13; // 8k
    private static final ConcurrentLinkedQueue<ByteBuffer> BUFFER_POOL = new ConcurrentLinkedQueue<>();

    private final MessagingProtocol<T> protocol;
    private final MessageEncoderDecoder<T> encdec;
    private final Queue<ByteBuffer> writeQueue = new ConcurrentLinkedQueue<>();
    private final SocketChannel chan;
    private final Reactor reactor;
    private final Connections<T> connections;
    private final int connectionId;

    public NonBlockingConnectionHandler(
            MessageEncoderDecoder<T> reader,
            MessagingProtocol<T> protocol,
            SocketChannel chan,
            Reactor reactor,
            Connections<T> connections,
            int connectionId) {
        this.chan = chan;
        this.encdec = reader;
        this.protocol = protocol;
        this.reactor = reactor;
        this.connections = connections;
        this.connectionId = connectionId;
        protocol.start(connectionId, connections,this);
    }



    @Override
    public void send(T msg) {
        if (msg == null) return;

        // Encode the message
        ByteBuffer encodedMsg = ByteBuffer.wrap(encdec.encode(msg));
        writeQueue.add(encodedMsg); // Add to the write queue
    
        // Ensure the channel is set to write mode
        reactor.updateInterestedOps(chan, SelectionKey.OP_READ | SelectionKey.OP_WRITE);
    }

    /*
    @Override
    public void send(T msg) {
        if (msg != null) {
            ByteBuffer buffer = ByteBuffer.wrap(encdec.encode(msg));
            synchronized (this) {
                writeQueue.add(buffer);
                reactor.updateInterestedOps(chan, SelectionKey.OP_WRITE);
            }
        }
    }
        */
        

    public Runnable continueRead() {
    ByteBuffer buf = leaseBuffer();

    boolean success = false;
    try {
        success = chan.read(buf) != -1;
    } catch (IOException ex) {
        ex.printStackTrace();
    }

    if (success) {
        buf.flip();
        return () -> {
            try {
                while (buf.hasRemaining()) {
                    T nextMessage = encdec.decodeNextByte(buf.get());
                    if (nextMessage != null) {
                        protocol.process(nextMessage); // Process the message
                        
                        // Handle termination if the protocol indicates it
                        if (protocol.shouldTerminate()) {
                            close();
                        }
                    }
                }
            } finally {
                releaseBuffer(buf);
            }
        };
    } else {
        releaseBuffer(buf);
        close();
        return null;
    }
}


    public void handleWrite() throws IOException {
        while (!writeQueue.isEmpty()) {
            ByteBuffer top = writeQueue.peek();
            chan.write(top);
            if (top.hasRemaining()) {
                return;
            } else {
                writeQueue.poll();
            }
        }
        reactor.updateInterestedOps(chan, SelectionKey.OP_READ);
    }

    @Override
    public void close() {
        try {
            chan.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        connections.disconnect(connectionId); //??
    }

    private static ByteBuffer leaseBuffer() {
        ByteBuffer buff = BUFFER_POOL.poll();
        if (buff == null) {
            return ByteBuffer.allocateDirect(BUFFER_ALLOCATION_SIZE);
        }
        buff.clear();
        return buff;
    }

    private static void releaseBuffer(ByteBuffer buff) {
        BUFFER_POOL.add(buff);
    }

    public MessagingProtocol<T> getProtocol() {
        return protocol;
    }
}