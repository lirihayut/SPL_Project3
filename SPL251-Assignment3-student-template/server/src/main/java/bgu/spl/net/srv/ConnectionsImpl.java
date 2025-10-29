package bgu.spl.net.srv;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;


public class ConnectionsImpl<T> implements Connections<T> {
    private Map<Integer, ConnectionHandler<T>> clients = new ConcurrentHashMap<>();
    private Map<String, CopyOnWriteArrayList<Integer>> channels = new ConcurrentHashMap<>(); 
    private Map<String, String> validCredentials = new ConcurrentHashMap<>();
    private Map<String, Integer> CurrentlyLoggedIn = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = clients.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String channel, T msg) {
        if(channels.containsKey(channel)){
            CopyOnWriteArrayList<Integer> subscribers = channels.get(channel);
            if (subscribers != null) {
                for (Integer id : subscribers) {
                    send(id, msg);
                }
            }
        }
    }


    @Override
    public void disconnect(int connectionId) {
        //clients.remove(connectionId);
        for (CopyOnWriteArrayList<Integer> subscribers : channels.values()) {
            subscribers.remove(Integer.valueOf(connectionId));
        }

        ConnectionHandler<T> connection = clients.remove(connectionId);
            if (connection != null) {
                try {
                    connection.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
    }


    @Override
    public void subscribeToChannel(int connectionId, String channel) {
        channels.putIfAbsent(channel, new CopyOnWriteArrayList<>());
        channels.get(channel).add(connectionId);
    }

    @Override
    public void unsubscribeFromChannel(int connectionId, String destination) {
        CopyOnWriteArrayList<Integer> subscribers = channels.get(destination);
        if (subscribers != null) {
            subscribers.remove(Integer.valueOf(connectionId));
            if (subscribers.isEmpty()) {
                channels.remove(destination); 
            }
        }
    }


    public boolean isCurrentlyLoggedIn(String login){
        return CurrentlyLoggedIn.containsKey(login);
    }

    public void loginUser(String login, int connectionId) {
        CurrentlyLoggedIn.putIfAbsent(login, connectionId) ;
    }

    public void logoutUser(int connectionId){
        CurrentlyLoggedIn.entrySet().removeIf(entry -> entry.getValue().equals(connectionId));
    }

    public void addClient(int connectionId, ConnectionHandler<T> handler) {
        clients.put(connectionId, handler);
    }


    @Override
    public boolean hasUser(String login) {
        return validCredentials.containsKey(login);
    }

    @Override
    public boolean isValidCredentials(String login, String passcode) {
        return validCredentials.containsKey(login) && validCredentials.get(login).equals(passcode);
    }

    @Override
    public void addValidCredentials(String login, String passcode) {
        validCredentials.putIfAbsent(login, passcode);
    }


}