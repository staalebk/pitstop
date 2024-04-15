import { Grid, Title } from "@mantine/core";
import { CarStateType } from "../commonTypes.ts";
import BrakeDisplay from "./BrakeDisplay.tsx";
import MapDisplay from "./MapDisplay.tsx";
import SpeedDisplay from "./SpeedDisplay.tsx";

const CarDisplay = ({ carState }: { carState: CarStateType }) => {
  return (
    <>
      <Title>Car {carState.id}</Title>
      <Grid w={1300}>
        <Grid.Col span={4}>
          <MapDisplay lat={carState.position[0]} lng={carState.position[1]} />
        </Grid.Col>
        <Grid.Col span={2}>
          <SpeedDisplay speed={carState.speed} />
        </Grid.Col>
        <Grid.Col span={2}>
          <BrakeDisplay label="left" brakeTemp={carState.brake_temp_left} />
        </Grid.Col>
        <Grid.Col span={2}>
          <BrakeDisplay label="right" brakeTemp={carState.brake_temp_right} />
        </Grid.Col>
      </Grid>
    </>
  );
};

export default CarDisplay;
