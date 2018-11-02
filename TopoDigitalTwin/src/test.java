import java.io.IOException;

public class test {
    public static void main(String[] args) throws IOException {
     TopoDigitalTwin twin= new TopoDigitalTwin();
     twin.getLayout();
    // System.out.println(twin.nodeList.get(1).getType());
     System.out.println(twin.connectionMap);
    }
}