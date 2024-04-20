import { useEffect, useState } from "react";
import useWebSocket, { ReadyState } from "react-use-websocket";
import { PAGE, RaceStateType } from "../commonTypes.ts";
import CarDisplay from "./CarDisplay.tsx";

type RaceViewProps = {
  setPage: (page: PAGE) => void;
  setConnectionState: (connectionState: ReadyState) => void;
  setTime: (time: number | null) => void;
};

const WS_URL =
  import.meta.env.MODE === "development"
    ? "ws://192.168.1.110:8888"
    : "wss://pitstop.driftfun.no:8888";

const RaceView = ({ setPage, setConnectionState, setTime }: RaceViewProps) => {
  const [raceState, setRaceState] = useState<RaceStateType>([]);

  const { readyState } = useWebSocket(WS_URL, {
    share: true,
    shouldReconnect: () => true,
    reconnectInterval: 3000,
    onMessage: (event) => {
      const parsedData = JSON.parse(event.data);
      // console.log("Received: ", parsedData);
      setRaceState(parsedData);
      setTime(parsedData[0].timestamp);
    },
    onClose: () => {
      setTime(null);
      setPage(PAGE.INIT);
      setConnectionState(ReadyState.CLOSED);
    },
  });

  useEffect(() => {
    setConnectionState(readyState);
  }, [readyState]);

  return (
    <>
      {raceState.length === 0 && <div>No cars connected yet...</div>}
      {raceState.map((carState) => (
        <CarDisplay key={carState.id} carState={carState} />
      ))}
    </>
  );
};

export default RaceView;
