package bgu.spl.net.impl.stomp;

import java.util.Map;

public class StompMessage {
    private final String command;
    private final Map<String, String> headers;
    private final String body;

    public StompMessage(String command, Map<String, String> headers, String body) {
        this.command = command;
        this.headers = headers;
        this.body = body;
    }

    public String getCommand() {
        return command;
    }

    public Map<String, String> getHeaders() {
        return headers;
    }

    public String getBody() {
        return body;
    }

    @Override
    public String toString() {
        return "Command: " + command + "\nHeaders: " + headers + "\nBody: " + body;
    }
}

