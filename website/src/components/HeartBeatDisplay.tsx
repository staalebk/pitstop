import { Text } from "@mantine/core";
import Speedometer, {
  Arc,
  Background,
  DangerPath,
  Indicator,
  Marks,
  Needle,
} from "react-speedometer/dist";

const HeartBeatDisplay = ({ heartrate }: { heartrate: number }) => {
  return (
    <div>
      <Text>Heart Rate</Text>
      <Speedometer value={heartrate} max={180} width={200}>
        <Background />
        <Arc />
        <Needle />
        <DangerPath from={150} to={180} />
        <Marks step={20} fontSize={15} />
        <Indicator color="white">
          {(value, textProps) => (
            <text
              {...textProps}
              fontSize={25}
              fontFamily="Inter"
              fill="white"
              x={200 / 2}
              y={200 / 2 + 70}
              textAnchor="middle"
              alignmentBaseline="middle"
            >
              {value} BPM
            </text>
          )}
        </Indicator>
      </Speedometer>
    </div>
  );
};

export default HeartBeatDisplay;
