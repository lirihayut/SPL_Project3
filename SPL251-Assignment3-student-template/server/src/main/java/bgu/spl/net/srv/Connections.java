package bgu.spl.net.srv;

import java.io.IOException;

import bgu.spl.net.impl.stomp.StompMessage;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    void unsubscribeFromChannel(int connectionId, String destination);

    void subscribeToChannel(int connectionId, String destination);

    void addClient(int connectionId, ConnectionHandler<T> handler);

    void addValidCredentials(String username, String password);

    boolean hasUser(String username);

    boolean isValidCredentials(String username, String password);

    boolean isCurrentlyLoggedIn(String login);

    void loginUser(String login, int connectionId);

    void logoutUser(int connectionId);
}
