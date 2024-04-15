import { ReadyState } from "react-use-websocket";

export const connectionStatus: Record<ReadyState, string> = {
  [ReadyState.CONNECTING]: "CONNECTING",
  [ReadyState.OPEN]: "CONNECTED",
  [ReadyState.CLOSING]: "CLOSING",
  [ReadyState.CLOSED]: "DISCONNECTED",
  [ReadyState.UNINSTANTIATED]: "START UP",
};
