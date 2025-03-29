import { map, pickBy, size } from "lodash-es";
import { useEffect, useState } from "react";
import useWebSocket, { ReadyState } from "react-use-websocket";
import { IncomingPacket, PAGE, RaceStateType } from "../commonTypes.ts";
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
  const [raceState, setRaceState] = useState<RaceStateType>({});

  const { readyState } = useWebSocket(WS_URL, {
    share: true,
    shouldReconnect: () => true,
    reconnectInterval: 3000,
    onMessage: (event) => {
      const parsedData = JSON.parse(event.data) as IncomingPacket;

      console.log("Received: ", parsedData);

      const currentTime = Date.now();
      const carsUpdatedLast5Minutes = pickBy(raceState, (car) => {
        return currentTime - car.timestamp > 5 * 60 * 1000;
      });

      const newRaceState: RaceStateType = {
        ...carsUpdatedLast5Minutes,
        [parsedData.uuid]: {
          ...parsedData.vehicle,
        },
      };

      setRaceState(newRaceState);
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

  console.log("RaceState: ", raceState);

  return (
    <>
      {size(raceState) === 0 && <div>No cars connected yet...</div>}
      {map(raceState, (carState, uuid) => (
        <CarDisplay key={uuid} id={uuid} carState={carState} />
      ))}
    </>
  );
};

export default RaceView;
