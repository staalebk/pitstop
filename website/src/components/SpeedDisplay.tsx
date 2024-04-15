import { Text } from "@mantine/core";
import Speedometer, {
  Arc,
  Background,
  Indicator,
  Marks,
  Needle,
} from "react-speedometer/dist";

const SpeedDisplay = ({ speed }: { speed: number }) => {
  return (
    <div>
      <Text>Speed</Text>
      <Speedometer value={speed} max={220} width={200}>
        <Background />
        <Arc />
        <Needle />
        <Marks fontSize={15} />
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
              {value} kph
            </text>
          )}
        </Indicator>
      </Speedometer>
    </div>
  );
};

export default SpeedDisplay;
