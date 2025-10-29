package bgu.spl.net.srv;

import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.MessageEncoderDecoder;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Supplier;

public abstract class BaseServer<T> implements Server<T> {

    private final int port;
    private final Supplier<MessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> encdecFactory;
    private final ConnectionsImpl<T> connections = new ConnectionsImpl<>();
    private ServerSocket sock;
    private final AtomicInteger connectionIdCounter = new AtomicInteger(0);

    public BaseServer(
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encdecFactory) {
        this.port = port;
        this.protocolFactory = protocolFactory;
        this.encdecFactory = encdecFactory;
        this.sock = null;
    }

    @Override
    public void serve() {

        try (ServerSocket serverSock = new ServerSocket(port)) {
			System.out.println("Server started");

            this.sock = serverSock; 

            while (!Thread.currentThread().isInterrupted()) {
                Socket clientSock = serverSock.accept();
                BlockingConnectionHandler<T> handler = (BlockingConnectionHandler<T>) createHandler(clientSock, connectionIdCounter.getAndIncrement());
                execute(handler);
            }
        } catch (IOException ex) {
        }
    }

    @Override
    public void close() throws IOException {
        if (sock != null) {
            sock.close();
        }
    }

    protected abstract void execute(ConnectionHandler<T> handler);

    protected abstract ConnectionHandler<T> createHandler(Socket clientSock, int connectionId);

    protected Connections<T> getConnections() {
        return connections;
    }
}