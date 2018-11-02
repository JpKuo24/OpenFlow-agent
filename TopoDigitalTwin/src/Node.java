import java.lang.*;

public interface Node {
    /**
     * @return ID of the node
     */
    String getId();

    /**
     * @return type of the node
     */
    String getType();
    /**
      @return heartbeat of the node
     */
    Boolean getHeartbeat();
}
