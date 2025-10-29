package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;

public class StompMessageEncoderDecoder implements MessageEncoderDecoder<StompMessage> {

    private StringBuilder buffer = new StringBuilder(); // Accumulate bytes until a complete message

    @Override
    public StompMessage decodeNextByte(byte nextByte) {
        if (nextByte == '\0') { // Null terminator indicates end of a STOMP frame
            StompMessage message = parseFrame(buffer.toString());
            buffer.setLength(0); // Reset buffer for the next message
            //System.out.println("MESSAGE RECIEVED ----------- " + message); 
            return message;
        } else {
            buffer.append((char) nextByte); // Append byte to the buffer
            return null; // Frame is not complete yet
        }
    }

    @Override
    public byte[] encode(StompMessage message) {
        String frame = encodeFrame(message);
        return frame.getBytes(StandardCharsets.UTF_8); // Convert the frame to a byte array
    }

    /**
     * Parses a STOMP frame from its string representation.
     */
    private StompMessage parseFrame(String frame) {
        String[] lines = frame.split("\n");
        String command = lines[0]; // The first line is the command

        Map<String, String> headers = new HashMap<>();
        int i = 1;
        // Parse headers until an empty line is encountered
        while (i < lines.length && !lines[i].isEmpty()) {
            String[] headerParts = lines[i].split(":", 2);
            headers.put(headerParts[0], headerParts[1]);
            i++;
        }

        // Remaining lines (after headers and the blank line) form the body
        StringBuilder body = new StringBuilder();
        i++; // Skip the blank line
        while (i < lines.length) {
            body.append(lines[i]).append("\n");
            i++;
        }

        return new StompMessage(command, headers, body.toString().trim());
    }

    /**
     * Encodes a STOMP message into its string representation.
     */
    private String encodeFrame(StompMessage message) {
        StringBuilder frame = new StringBuilder();
        frame.append(message.getCommand()).append("\n");

        for (Map.Entry<String, String> entry : message.getHeaders().entrySet()) {
            frame.append(entry.getKey()).append(":").append(entry.getValue()).append("\n");
        }

        frame.append("\n").append(message.getBody()).append("\0"); // Null terminator
        return frame.toString();
    }
}
