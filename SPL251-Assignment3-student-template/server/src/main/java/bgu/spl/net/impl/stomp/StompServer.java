package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: java StompServer <port> <reactor|tpc>");
            return;
        }

        int port;
        try {
            port = Integer.parseInt(args[0]);
        } catch (NumberFormatException e) {
            System.out.println("Invalid port number: " + args[0]);
            return;
        }

        String serverType = args[1];
        ConnectionsImpl<StompMessage> connections = new ConnectionsImpl<>();

        if (serverType.equalsIgnoreCase("tpc")) {
            Server.threadPerClient(
                port,
                StompMessagingProtocolImpl::new,  // Implement StompMessagingProtocol
                StompMessageEncoderDecoder::new
            ).serve();
        } else if (serverType.equalsIgnoreCase("reactor")) {
            Server.reactor(
                Runtime.getRuntime().availableProcessors(),
                port,
                StompMessagingProtocolImpl::new,  // Implement StompMessagingProtocol
                StompMessageEncoderDecoder::new,
                connections  // Implement Stomp Encoder/Decoder
            ).serve();
        } else {
            System.out.println("Invalid server type. Use 'tpc' for Thread-Per-Client or 'reactor' for Reactor.");
        }
    }
}
