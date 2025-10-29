package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import java.io.Closeable;
import java.io.IOException;
import java.net.Socket;
import java.util.function.Supplier;

public interface Server<T> extends Closeable {
    void serve();
    static <T> Server<T> threadPerClient(
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encoderDecoderFactory) {
        return new BaseServer<T>(port, protocolFactory, encoderDecoderFactory) {

            @Override
            protected void execute(ConnectionHandler<T> handler) {
                // Start the handler in a new thread
                new Thread((Runnable) handler).start();
            }

            @Override
            protected ConnectionHandler<T> createHandler(Socket clientSock, int connectionId) {
                return new BlockingConnectionHandler<>(
                        clientSock,
                        encoderDecoderFactory.get(),
                        protocolFactory.get(),
                        getConnections(),
                        connectionId);
            }
        };
    }

    static <T> Server<T> reactor(
            int nthreads,
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encoderDecoderFactory,
            Connections<T> connections) {
        return new Reactor<T>(nthreads, port, protocolFactory, encoderDecoderFactory,connections);
    }
}