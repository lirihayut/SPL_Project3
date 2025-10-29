package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.channels.*;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.function.Supplier;

public class Reactor<T> implements Server<T> {

    private final int port;
    private final Supplier<MessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> readerFactory;
    private final ActorThreadPool pool;
    private Selector selector;

    private Thread selectorThread;
    private final ConcurrentLinkedQueue<Runnable> selectorTasks = new ConcurrentLinkedQueue<>();

    // Use ConnectionsImpl to manage active client connections
    private final Connections<T> connections;
    private int connectionIdCounter = 0;

    public Reactor(
            int numThreads,
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> readerFactory,
            Connections<T> connections) {

        this.pool = new ActorThreadPool(numThreads);
        this.port = port;
        this.protocolFactory = protocolFactory;
        this.readerFactory = readerFactory;
        this.connections = connections;
    }

    @Override
    public void serve() {
        selectorThread = Thread.currentThread();
        try (Selector selector = Selector.open();
             ServerSocketChannel serverSock = ServerSocketChannel.open()) {

            this.selector = selector;

            // Bind and configure the server socket
            serverSock.bind(new InetSocketAddress(port));
            serverSock.configureBlocking(false);
            serverSock.register(selector, SelectionKey.OP_ACCEPT);

            System.out.println("Server started");

            while (!Thread.currentThread().isInterrupted()) {
                selector.select();
                runSelectionThreadTasks();

                for (SelectionKey key : selector.selectedKeys()) {
                    if (!key.isValid()) {
                        continue;
                    } else if (key.isAcceptable()) {
                        handleAccept(serverSock, selector);
                    } else {
                        handleReadWrite(key);
                    }
                }

                selector.selectedKeys().clear(); // Clear selected keys for the next cycle
            }

        } catch (ClosedSelectorException ex) {
            // Do nothing - server was requested to be closed
        } catch (IOException ex) {
            // Handle IOExceptions properly
            ex.printStackTrace();
        }

        System.out.println("Server closed!!!");
        pool.shutdown();
    }

    /**
     * Update the operations of a given channel.
     */
    void updateInterestedOps(SocketChannel chan, int ops) {
        final SelectionKey key = chan.keyFor(selector);
        if (Thread.currentThread() == selectorThread) {
            key.interestOps(ops);
        } else {
            selectorTasks.add(() -> key.interestOps(ops));
            selector.wakeup();
        }
    }

    /**
     * Handle incoming client connections.
     */
    private void handleAccept(ServerSocketChannel serverChan, Selector selector) throws IOException {
        SocketChannel clientChan = serverChan.accept();
        clientChan.configureBlocking(false);
    
        // Assign a new connection ID
        int connectionId = connectionIdCounter++;
    
        // Create a handler for the new client
        NonBlockingConnectionHandler<T> handler = new NonBlockingConnectionHandler<>(
                readerFactory.get(),
                protocolFactory.get(),
                clientChan,
                this,
                connections,
                connectionId
        );
    
        // Add the client to the Connections map
        connections.addClient(connectionId, handler);
    
        // Register the client channel with the selector
        clientChan.register(selector, SelectionKey.OP_READ, handler);
    }

    /**
     * Handle read and write operations for a given key.
     */
    private void handleReadWrite(SelectionKey key) {
        @SuppressWarnings("unchecked")
        NonBlockingConnectionHandler<T> handler = (NonBlockingConnectionHandler<T>) key.attachment();

        // If the channel is readable, process reading
        if (key.isReadable()) {
            Runnable task = handler.continueRead();
            if (task != null) {
                pool.submit(handler, task);
            }
        }

        // If the channel is writable, process writing
        if (key.isValid() && key.isWritable()) {
            try {
                handler.handleWrite();
            } catch (IOException e) {
                e.printStackTrace();
                key.cancel();
            }
        }
    }

    /**
     * Execute tasks that are added by other threads.
     */
    private void runSelectionThreadTasks() {
        while (!selectorTasks.isEmpty()) {
            selectorTasks.remove().run();
        }
    }

    /**
     * Close the server and its resources.
     */
    @Override
    public void close() throws IOException {
        selector.close();
    }

    /**
     * Send a message to a specific connection ID.
     */
    public boolean sendToClient(int connectionId, T msg) {
        return connections.send(connectionId, msg);
    }

    /**
     * Send a message to all subscribed clients.
     */
    public void sendToChannel(String channel, T msg) {
        connections.send(channel, msg);
    }

    /**
     * Disconnect a client from the server.
     */
    public void disconnectClient(int connectionId) {
        connections.disconnect(connectionId);
    }
}
