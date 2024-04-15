import { Badge } from "@mantine/core";
import { ReadyState } from "react-use-websocket";
import { connectionStatus } from "../utils/utils.tsx";

const badgeColor = {
  [ReadyState.CONNECTING]: "yellow",
  [ReadyState.OPEN]: "green",
  [ReadyState.CLOSING]: "yellow",
  [ReadyState.CLOSED]: "red",
  [ReadyState.UNINSTANTIATED]: "grey",
};

type StatusBaseProps = {
  connectionState: ReadyState;
};

const StatusBadge = ({ connectionState }: StatusBaseProps) => (
  <Badge mt={20} me={10} color={badgeColor[connectionState]}>
    {connectionStatus[connectionState]}
  </Badge>
);

export default StatusBadge;
