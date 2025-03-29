import { Grid, Title } from "@mantine/core";
import { CarStateType } from "../commonTypes.ts";
import HeartBeatDisplay from "./HeartBeatDisplay.tsx";
import MapDisplay from "./MapDisplay.tsx";
import RPMDisplay from "./RPMDisplay.tsx";
import SpeedDisplay from "./SpeedDisplay.tsx";

type CarDisplayProps = {
  carState: CarStateType;
  id: string;
};

const CarDisplay = ({ carState, id }: CarDisplayProps) => {
  return (
    <>
      <Title>Car {id}</Title>
      <Grid w={{ base: "100%", md: 1300 }}>
        <Grid.Col span={{ base: 12, lg: 6 }}>
          <MapDisplay
            heading={carState.heading}
            lat={carState.latitude}
            lng={carState.longitude}
          />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <SpeedDisplay speed={carState.speed} />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <RPMDisplay rpm={carState.rpm} />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <HeartBeatDisplay heartrate={carState.heart_rate} />
        </Grid.Col>
      </Grid>
    </>
  );
};

export default CarDisplay;
