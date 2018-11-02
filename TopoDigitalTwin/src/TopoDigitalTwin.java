import com.mongodb.MongoClient;
import com.mongodb.client.MongoCollection;
import com.mongodb.client.MongoDatabase;
import org.bson.Document;


import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.LineNumberReader;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


public class TopoDigitalTwin {
    public static List<Node> nodeList = new ArrayList<Node>(); // Store all the nodes in the plant floor
    static Map<Node,List<Node>> connectionMap = new HashMap<Node,List<Node>>();  //Store the link relationship between different nodes
    String originFileName = "./resources/config/initialLayout.txt";
    String fileName = "./resources/config/Layout.txt";
    int nodeCount=0;

    /*
     * Get the layout
     */
    public void getLayout() throws IOException {
        File originFile = new File(originFileName);
        File file = new File(fileName);

        //copy the file to another directory for updating without changing the initial topology file
        if (originFile.exists() && originFile.isDirectory()) {
            Files.copy(originFile.toPath(), file.toPath(), StandardCopyOption.REPLACE_EXISTING);
        }

        //get Net_Nodes
        FileReader fr = new FileReader(file);
        LineNumberReader lnr = new LineNumberReader(fr);
        int lineNumber = 0;
        while (lnr.readLine() != null) {
            lineNumber++;
        }
        nodeCount = lineNumber;
        getAllNodes();

    }

    /*
     * fill up the nodeList and connectionMap
     */
    public List<Node> getAllNodes() throws IOException {
        File file = new File(fileName);
        FileReader fr = new FileReader(file);
        LineNumberReader lnr = new LineNumberReader(fr);
        List<String> temp = new ArrayList<String>();
        for (int i = 0; i < nodeCount; i++) {
            List<Node> connectList = new ArrayList<>();
            temp.add(lnr.readLine());
            String nodeid =temp.get(i).substring(0,temp.get(i).indexOf(" "));
            putNodeintoList(nodeList, nodeid);
            for (String a : temp.get(i).split("\\s+")) {
                if(a.contains(":")){
                    connectList=putNodeintoList(connectList,a.substring(a.indexOf(":")+1));
                }
            }
            connectionMap.put(nodeList.get(i),connectList);
        }

        return nodeList;
    }

    /*
    * Helper function to instance corresponding nodes and stores these nodes in a link
     */
    public List<Node> putNodeintoList(List<Node> list, String nodeid){
        if(nodeid.charAt(0)=='c') {
            list.add(new CncNode(nodeid));
        }
        else if(nodeid.charAt(0)=='s') {
            list.add(new InterNode("Stopper", nodeid, true));
        }
        else if(nodeid.charAt(0)=='r')
            list.add(new InterNode("Robot", nodeid, true));
        return list;
    }

}
