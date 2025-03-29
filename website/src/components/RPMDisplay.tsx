import { Text } from "@mantine/core";
import Speedometer, {
  Arc,
  Background,
  DangerPath,
  Indicator,
  Marks,
  Needle,
} from "react-speedometer/dist";

const RPMDisplay = ({ rpm }: { rpm: number }) => {
  return (
    <div>
      <Text>RPM</Text>
      <Speedometer value={rpm} max={7000} width={200}>
        <Background />
        <Arc />
        <Needle />
        <DangerPath angle={32} />
        <Marks step={500} fontSize={15} />
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
              {value} RPM
            </text>
          )}
        </Indicator>
      </Speedometer>
    </div>
  );
};

export default RPMDisplay;
