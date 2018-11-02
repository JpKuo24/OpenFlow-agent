public abstract class EndNode implements Node {
    private Boolean heartbeat;
    private String ipAdd;
    private final String macAdd;
    private String program;
    private final String endNodeType;
    private final String endNodeId;

    // For serialization
    public EndNode(String type,String id,String ipAdd,String macAdd,String program,Boolean heartbeat) {
        this.endNodeType = type;
        this.endNodeId = id;
        this.ipAdd = ipAdd;
        this.macAdd = macAdd;
        this.program = program;
        this.heartbeat = heartbeat;
    }

    @Override
    public String getId() { return endNodeId; }

    @Override
    public String getType() {
        return endNodeType;
    }

    @Override
    public Boolean getHeartbeat() {
        return heartbeat;
    }

    public String getIpAdd() {return ipAdd;};

    public String getMacAdd() {return macAdd;};

    public String getProgram() {return program;}

    public void setHeartbeat(Boolean heartbeat){
        this.heartbeat = heartbeat;
    }

    public void setIpAdd(String ipAdd){
        this.ipAdd= ipAdd;
    }

    public void setProgram(String program){
        this.program = program;
    }

}
