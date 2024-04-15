import { Alert, Tooltip } from "@mantine/core";
import { clamp, upperFirst } from "@mantine/hooks";
import { SixteenNumberArray } from "../commonTypes.ts";

type BrakeDisplayProps = {
  brakeTemp: SixteenNumberArray;
  label: string;
};

// Function to map temperature to a color
const getTempColor = (temp: number) => {
  const maxTemp = 900;
  const minTemp = 90;
  const normalizedTemp = clamp((temp - minTemp) / (maxTemp - minTemp), 0, 1); // Normalize to 0-1
  const hue = (1 - normalizedTemp) * 120; // Hue for green is 120, for red is 0
  return `hsl(${hue}, 100%, 50%)`;
};

/**
 * Displays brake temperature for a wheel. Temperature rendered so the first temperature in array is the innermost
 * circle, and the last temperature in the array is the outermost circle.
 */
const BrakeDisplay = ({ brakeTemp, label }: BrakeDisplayProps) => {
  if (brakeTemp == null || brakeTemp.length !== 16) {
    return (
      <Alert w={200} h={200}>
        Waiting for brake data for {label} wheel
      </Alert>
    );
  }

  const maxTemp = Math.max(...brakeTemp);

  return (
    <div>
      <div>{upperFirst(label)} brake temperature</div>
      <svg width="200" height="200">
        <Tooltip.Group openDelay={500} closeDelay={100}>
          {brakeTemp
            .slice()
            .reverse()
            .map((temp, index) => (
              <Tooltip key={index} label={`${temp} °C`} withArrow>
                <circle
                  cx="100" // Center the circles
                  cy="100"
                  r={100 - index * 5} // Decrease the radius for each circle
                  fill={getTempColor(temp)}
                />
              </Tooltip>
            ))}
        </Tooltip.Group>
        <circle cx="100" cy="100" r="20" fill="white" />
        <text
          x={100}
          y={100}
          fontSize={12}
          textAnchor="middle"
          dominantBaseline="middle"
        >
          {maxTemp}°C
        </text>
      </svg>
    </div>
  );
};

export default BrakeDisplay;
