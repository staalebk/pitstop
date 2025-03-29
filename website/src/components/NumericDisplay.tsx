import { RingProgress, RingProgressProps, Text } from "@mantine/core";

type NumericDisplayProps = {
  label: string;
  unit: string;
  max: number;
  value: number;
  alertOver?: number;
};

const backgroundColor = "#e9ecef";

const NumericDisplay = ({
  label,
  value,
  max,
  unit,
  alertOver,
}: NumericDisplayProps) => {
  const sections: RingProgressProps["sections"] = [];

  const color = alertOver && value > alertOver ? "red" : "blue";

  const amountOverHalf = value - max / 2;
  if (amountOverHalf > 0) {
    sections.push({ value: (amountOverHalf / max) * 100, color });
    sections.push({
      value: 50 - (amountOverHalf / max) * 100,
      color: backgroundColor,
    });
  } else {
    sections.push({ value: 50, color: backgroundColor });
  }
  sections.push({ value: (value / max) * 100, color });

  return (
    <RingProgress
      rootColor={backgroundColor}
      label={
        <>
          <Text size={"xs"} c={color} ta="center">
            {label}
          </Text>
          <Text ta="center">
            {value}
            {unit}
          </Text>
        </>
      }
      sections={sections}
    />
  );
};

export default NumericDisplay;
