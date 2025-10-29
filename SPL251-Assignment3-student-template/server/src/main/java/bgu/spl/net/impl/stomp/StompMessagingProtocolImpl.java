package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.Connections;


import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class StompMessagingProtocolImpl<T> implements StompMessagingProtocol<T> {
    private int connectionId; 
    private Connections<T> connections;
    private boolean shouldTerminate; 
    private Map<String, Integer> subscriptions; 
    private boolean isConnected;
    private static final AtomicInteger messageCounter = new AtomicInteger(0);
    private ConnectionHandler<T> handler;
    private static final String SUPPORTED_VERSION = "1.2";
    private static final String VALID_HOST = "stomp.cs.bgu.ac.il";

    public StompMessagingProtocolImpl() {
        this.shouldTerminate = false;
        this.subscriptions = new ConcurrentHashMap<>();
        this.isConnected = false;
    }

    @Override
    public void start(int connectionId, Connections<T> connections, ConnectionHandler<T> handler) {
        this.connectionId = connectionId;
        this.connections = connections;
        this.handler = handler;
    }


    @Override
    public void process(T message) {
        if (message instanceof StompMessage) {
            StompMessage frame = (StompMessage) message;
            String command = frame.getCommand();
            System.out.println("MESSAGE RECIEVED: -------"+frame);

            switch (command) {
                case "CONNECT":
                    handleConnect(frame);
                    break;
                case "SEND":
                    handleSend(frame);
                    break;
                case "SUBSCRIBE":
                    handleSubscribe(frame);
                    break;
                case "UNSUBSCRIBE":
                    handleUnsubscribe(frame);
                    break;
                case "DISCONNECT":
                    handleDisconnect(frame);
                    break;
                default:
                    handleError("Unknown command: " + command, "The command "+ command +" is not recognized");
                    break;
            }
        }
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }



    private void handleConnect(StompMessage msg) {
        if (isConnected) { 
            handleError("Client is already connected", "client is already connected, logout before trying to login");
            return;
        }
        
        String version = msg.getHeaders().get("accept-version");
        String host = msg.getHeaders().get("host");

        if (version == null || !version.equals(SUPPORTED_VERSION)) {
            handleError("accept-version is invalid or missing", "version is invalid or missing, expected version: "+SUPPORTED_VERSION);
            return;
        }

        if (host == null || !host.equals(VALID_HOST)) {
            handleError("host is invalid or missing" ,"host is invalid or missing, expected host: "+VALID_HOST);
            return;
        }

        String login = msg.getHeaders().get("login");
        String passcode = msg.getHeaders().get("passcode");

        if (login == null || passcode == null) {
            handleError("login or passcode are missing", "need to enter login and passcode");
            return;
        }

        if (connections.hasUser(login)) { //user already exists
            if (!connections.isValidCredentials(login, passcode)) { //password doesn't match
                handleError("Incorrect password" , "Incorrect password for existing user: " + login);
                return;
            }
            if(connections.isCurrentlyLoggedIn(login)){
                handleError("User is already logged in", "User is already logged in");
                return;
            }
        } 
            connections.addValidCredentials(login, passcode);
            connections.loginUser(login, connectionId);
            connections.addClient(connectionId, handler);
        

        isConnected = true;
        sendConnectedFrame(version);
        System.out.println("client connected successfuly");
    }



    private void handleSubscribe(StompMessage msg) {
        String destination = "/" + msg.getHeaders().get("destination");
        String id = msg.getHeaders().get("id");

        if (destination == null || id == null) {
            handleError("destination or id are missing", "destination or id are null");
            return;
        }

        if (subscriptions.containsKey(destination)) {
            return; //already subscribed
        }
        
        try {
            int subscriptionId = Integer.parseInt(id);
            subscriptions.put(destination, subscriptionId);
            connections.subscribeToChannel(connectionId, destination);
            System.out.println("subscribed to channel: "+destination);
        } catch (NumberFormatException e) {
            handleError("Invalid 'id': must be an integer", "id must be an integer");
            return;
        }
        
    }



    private void handleUnsubscribe(StompMessage msg) {
        String id = msg.getHeaders().get("id");

        if (id == null) {
            handleError("id is missing", "id is missing");
            return;
        }
        
        try {
            int subscriptionId = Integer.parseInt(id);
            String destination = null;
            for (String d : subscriptions.keySet()) {
                if (subscriptions.get(d).equals(subscriptionId)) {
                    destination = d;
                    break;
                }
            }
            if (destination == null) {
                return;
            }
            subscriptions.remove(destination);
            connections.unsubscribeFromChannel(connectionId, destination);
            System.out.println("unsubscribed from channel: "+destination);
        } catch (NumberFormatException e) {
            handleError("id must be an integer","id must be an integer");
            return;
        }

        
    }


    private void handleSend(StompMessage msg) {
        String destination = msg.getHeaders().get("destination");
        if (destination == null || destination.isEmpty()) {
            handleError("destination header is empty in SEND frame","The SEND frame must contain a destination header");
            return;
        }
        String body = msg.getBody();
        if (body == null || body.isEmpty()) {
            handleError("message body is empty in SEND frame","The SEND frame must contain a non-empty message body");
            return;
        }
        if (!subscriptions.containsKey(destination)) {
            handleError("Client is not subscribed to: " + destination, "Client is not subscribed to: " + destination);
            return;
        }
        String user =  msg.getHeaders().get("user");
        if (user == null || user.isEmpty()) {
            handleError("user not found in message body", "The SEND frame must contain a 'user' in the message body");
            return;
        }

        int mesgId = messageCounter.incrementAndGet();
        Map<String, String> messageHeaders = new HashMap<>();
        messageHeaders.put("destination", destination);
        messageHeaders.put("subscription", subscriptions.get(destination).toString());
        messageHeaders.put("user", user);
        messageHeaders.put("message-id", String.valueOf(mesgId));
        StompMessage message = new StompMessage("MESSAGE", messageHeaders, body);
        connections.send(destination, (T) message);
        System.out.println("message sent to: "+destination);
    }


    private void handleDisconnect(StompMessage msg) {
        String receiptId = msg.getHeaders().get("receipt");
        if (receiptId == null) {
            handleError("receiptID is missing","receiptID is missing" );
            return;
        }
        if(!isConnected){
            handleError("user is not logged in","user is not logged in, need to login first" );
            return;
        }

        sendReceiptFrame(receiptId);
        shouldTerminate = true;
        //connections.disconnect(connectionId);
        connections.logoutUser(connectionId);

        try{
            Thread.sleep(100);
        } catch(InterruptedException e){
            e.printStackTrace();
        }
        isConnected=false;
        // Clear all subscriptions
        subscriptions.clear(); //!!!!!!!!!!!!!!!!!!!!!!
        System.out.println("disconnected");
    }

    private void handleError(String errorMessage, String detailedDescription) {
        Map<String, String> headers = new HashMap<>();
        headers.put("message", errorMessage);

        StompMessage errorFrame = new StompMessage("ERROR", headers, detailedDescription);
        connections.send(connectionId, (T) errorFrame);
        //shouldTerminate = true;
        //connections.disconnect(connectionId);
    }


    private void sendConnectedFrame(String version) {
        Map<String, String> headers = new HashMap<>();
        headers.put("version", version);
        StompMessage connectedFrame = new StompMessage("CONNECTED", headers, "");
        connections.send(connectionId, (T) connectedFrame);
    }

    private void sendReceiptFrame(String receiptId) {
        Map<String, String> headers = new HashMap<>();
        headers.put("receipt-id", receiptId);
        StompMessage receiptFrame = new StompMessage("RECEIPT", headers, "");
        connections.send(connectionId, (T) receiptFrame);
    }
}
