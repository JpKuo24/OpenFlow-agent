import java.util.List;
public class CncNode extends EndNode {
    private Boolean heartbeat;
    private String ipAdd;
    private String macAdd;
    private String program;
    private String type;
    private String id;
    private List<Drive> driveList;
    private String controlMode;
    public CncNode(String id){
        super("CNC",id,"","","", true);
    }
    public CncNode(String id,String ipAdd,String macAdd,String program,Boolean heartbeat,List<Drive> driveList,String controlmode){
       super("CNC",id,ipAdd,macAdd,program,heartbeat);
       this.driveList = driveList;
       this.controlMode = controlmode;
    }
}

class Drive {
    private String actionState;
    public Drive(String actionState){
        this.actionState = actionState;
    }

}