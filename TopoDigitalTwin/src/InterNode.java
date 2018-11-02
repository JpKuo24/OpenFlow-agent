public class InterNode implements Node{
    private Boolean heartbeat;
    private final String interNodeType;
    private final String interNodeId;

    // For serialization
    public InterNode(String type,String id,Boolean heartbeat) {
        this.interNodeType = type;
        this.interNodeId = id;
        this.heartbeat = heartbeat;
    }
    @Override
    public String getId() { return interNodeId; }

    @Override
    public String getType() {
        return interNodeType;
    }

    @Override
    public Boolean getHeartbeat() {
        return heartbeat;
    }
}
