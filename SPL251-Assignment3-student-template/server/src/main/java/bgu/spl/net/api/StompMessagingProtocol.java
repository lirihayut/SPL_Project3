package bgu.spl.net.api;

import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;

public interface StompMessagingProtocol<T> extends MessagingProtocol<T> {
    /**
     * Used to initiate the current client protocol with its personal connection ID and the connections implementation
     **/
    @Override
    void start(int connectionId, Connections<T> connections, ConnectionHandler<T> handler);

    /**
     * Process the given message
     * @param message the received message
     */
    @Override
    void process(T message);

    /**
     * @return true if the connection should be terminated
     */
    @Override
    boolean shouldTerminate();
}
