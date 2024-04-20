import { Grid, Title } from "@mantine/core";
import { CarStateType } from "../commonTypes.ts";
import HeartBeatDisplay from "./HeartBeatDisplay.tsx";
import MapDisplay from "./MapDisplay.tsx";
import RPMDisplay from "./RPMDisplay.tsx";
import SpeedDisplay from "./SpeedDisplay.tsx";

const CarDisplay = ({ carState }: { carState: CarStateType }) => {
  return (
    <>
      <Title>Car {carState.id}</Title>
      <Grid w={{ base: "100%", md: 1300 }}>
        <Grid.Col span={{ base: 12, lg: 6 }}>
          <MapDisplay
            heading={carState.heading}
            lat={carState.position[0]}
            lng={carState.position[1]}
          />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <SpeedDisplay speed={carState.speed} />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <RPMDisplay rpm={carState.rpm} />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <HeartBeatDisplay heartrate={carState.heartrate} />
        </Grid.Col>
        {/*<Grid.Col span={2}>
          <BrakeDisplay brakeTemp={carState.brake_temp} />
        </Grid.Col>*/}
      </Grid>
    </>
  );
};

export default CarDisplay;
