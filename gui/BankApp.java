import javax.json.*;
import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.io.*;
import java.net.*;

public class BankApp {

    private static JTextField accountIdField;
    private static JTextField nameField;
    private static JRadioButton depositBtn;
    private static JRadioButton withdrawBtn;
    private static JTextField amountField;
    private static JTextArea outputArea;

    public static void main(String[] args) {
        SwingUtilities.invokeLater(BankApp::buildUI);
    }

    private static void buildUI() {
        JFrame frame = new JFrame("Bank Transaction");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(600, 520);
        frame.setLocationRelativeTo(null);
        frame.setResizable(false);

        JPanel main = new JPanel(new BorderLayout(10, 10));
        main.setBorder(new EmptyBorder(20, 20, 20, 20));
        main.setBackground(new Color(30, 30, 40));

        JLabel title = new JLabel("Bank Transaction Portal");
        title.setFont(new Font("SansSerif", Font.BOLD, 20));
        title.setForeground(new Color(100, 200, 255));
        title.setHorizontalAlignment(SwingConstants.CENTER);
        title.setBorder(new EmptyBorder(0, 0, 10, 0));

        JPanel form = new JPanel(new GridLayout(4, 2, 10, 12));
        form.setBackground(new Color(30, 30, 40));

        accountIdField = styledField();
        nameField  = styledField();
        amountField = styledField();
        depositBtn  = styledRadio("DEPOSIT");
        withdrawBtn = styledRadio("WITHDRAWAL");

        ButtonGroup group = new ButtonGroup();
        group.add(depositBtn);
        group.add(withdrawBtn);

        JPanel radioPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 10, 0));
        radioPanel.setBackground(new Color(30, 30, 40)); 
        radioPanel.add(depositBtn);
        radioPanel.add(withdrawBtn);

        depositBtn  = styledRadio("DEPOSIT");
        withdrawBtn = styledRadio("WITHDRAWAL");
     // depositBtn.setSelected(true);
    


        String[] labels = {"Account ID", "Name", "Type", "Amount ($)"};
        JComponent[] inputs = {accountIdField, nameField, radioPanel, amountField};

        for (int i = 0; i < labels.length; i++) {
            JLabel lbl = new JLabel(labels[i]);
            lbl.setForeground(Color.LIGHT_GRAY);
            lbl.setFont(new Font("SansSerif", Font.PLAIN, 14));
            form.add(lbl);
            form.add(inputs[i]);
        }

        JButton sendButton = new JButton("Send Transaction");
        sendButton.setFont(new Font("SansSerif", Font.BOLD, 14));
        sendButton.setBackground(new Color(0, 120, 200));
        sendButton.setForeground(Color.WHITE);
        sendButton.setFocusPainted(false);
        sendButton.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        sendButton.setBorder(new EmptyBorder(10, 0, 10, 0));
        sendButton.addActionListener(e -> handleSend());

        outputArea = new JTextArea(6, 40);
        outputArea.setEditable(false);
        outputArea.setBackground(new Color(20, 20, 28));
        outputArea.setForeground(new Color(100, 220, 120));
        outputArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        outputArea.setBorder(new EmptyBorder(8, 8, 8, 8));
        JScrollPane scroll = new JScrollPane(outputArea);
        scroll.setBorder(BorderFactory.createLineBorder(new Color(60, 60, 80)));

        JPanel bottom = new JPanel(new BorderLayout(0, 8));
        bottom.setBackground(new Color(30, 30, 40));
        bottom.add(sendButton, BorderLayout.NORTH);
        bottom.add(scroll, BorderLayout.CENTER);

        main.add(title, BorderLayout.NORTH);
        main.add(form, BorderLayout.CENTER);
        main.add(bottom, BorderLayout.SOUTH);

        frame.setContentPane(main);
        frame.setVisible(true);
    }

    private static void handleSend() {
        String accountId  = accountIdField.getText().trim();
        String name = nameField.getText().trim();
        String type = depositBtn.isSelected() ? "DEPOSIT" : "WITHDRAWAL";
        String amountText = amountField.getText().trim();

        if (accountId.isEmpty() || name.isEmpty() || amountText.isEmpty()) {
            outputArea.setText("[ERROR] All fields are required.");
            return;
        }

        double amount;
        try {
            amount = Double.parseDouble(amountText);
            if (amount <= 0) throw new NumberFormatException();
        } catch (NumberFormatException ex) {
            outputArea.setText("[ERROR] Amount must be a positive number.");
            return;
        }

        JsonObject payload = Json.createObjectBuilder()
                .add("Account-ID", accountId)
                .add("Name",      name)
                .add("Type",      type)
                .add("Amount",    amount)
                .build();

        StringWriter sw = new StringWriter();
        try (JsonWriter jw = Json.createWriter(sw)) {
            jw.writeObject(payload);
        }

        String jsonp = sw.toString();
        sendJsonp(jsonp);
    }

    private static void sendJsonp(String jsonp) {
        //Socket programming here
        Socket socket = null;


        try{
            socket = new Socket("127.1.0.0", 8080);
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            out.println(jsonp);
            outputArea.setText("Connection successful");
            outputArea.setText("Sent: " + jsonp);
            String response = in.readLine(); 
    
            if (response != null) 
            {
                outputArea.setText("Received: " + response);
            } else 
            {
                outputArea.setText("Server closed connection without responding.");
            }
        }catch(UnknownHostException u){
            System.out.println(u);
            return;
        }catch(IOException i)
        {
            System.out.println(i);
            outputArea.setText("Connection failed");
            return;
        }finally{
            try
            {
                if(socket != null)
                {
                    socket.close();
                    System.out.println("Closed client socket");

                }
                
            }catch(IOException e)
            {
                System.err.println("There has been an error closing socket: " + e.getMessage());
            }
        }

     
            
    }

    private static JTextField styledField() {
        JTextField f = new JTextField();
        f.setBackground(new Color(50, 50, 65));
        f.setForeground(Color.WHITE);
        f.setFont(new Font("SansSerif", Font.PLAIN, 14));
        f.setBorder(BorderFactory.createLineBorder(new Color(80, 80, 100)));
        f.setCaretColor(Color.WHITE);
        return f;
    }

    private static JRadioButton styledRadio(String label) {
        JRadioButton r = new JRadioButton(label);
        r.setForeground(Color.WHITE);
        r.setBackground(new Color(30, 30, 40));
        r.setFont(new Font("SansSerif", Font.PLAIN, 14));
        r.setFocusPainted(false);
        return r;
    }
}