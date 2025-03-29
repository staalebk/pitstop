import { Center, Grid, Title } from "@mantine/core";
import { CarStateType } from "../commonTypes.ts";
import MapDisplay from "./MapDisplay.tsx";
import NumericDisplay from "./NumericDisplay.tsx";
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
      <Grid w={{ base: "100%" }}>
        <Grid.Col span={{ base: 12, lg: 6 }}>
          <MapDisplay
            heading={carState.heading}
            lat={carState.latitude}
            lng={carState.longitude}
          />
        </Grid.Col>
        <Grid.Col span={{ base: 6, lg: 2 }}>
          <SpeedDisplay speed={carState.speed} />
        </Grid.Col>
        <Grid.Col span={{ base: 6, lg: 2 }}>
          <RPMDisplay rpm={carState.rpm} />
        </Grid.Col>
        <Grid.Col span={{ base: 12, lg: 2 }}>
          <Grid>
            <Grid.Col span={6}>
              <Center>
                <NumericDisplay
                  value={carState.accelerator}
                  max={100}
                  unit="%"
                  label="Throttle"
                />
              </Center>
            </Grid.Col>
            <Grid.Col span={6}>
              <Center>
                <NumericDisplay
                  value={carState.brake}
                  max={100}
                  unit="%"
                  label="Brake"
                />
              </Center>
            </Grid.Col>
            <Grid.Col span={6}>
              <Center>
                <NumericDisplay
                  value={carState.coolant_temp}
                  unit="°C"
                  max={200}
                  label="Coolant"
                  alertOver={105}
                />
              </Center>
            </Grid.Col>
            <Grid.Col span={6}>
              <Center>
                <NumericDisplay
                  value={carState.oil_temp}
                  max={200}
                  unit="°C"
                  label="Oil"
                  alertOver={120}
                />
              </Center>
            </Grid.Col>
          </Grid>
        </Grid.Col>
      </Grid>
    </>
  );
};

export default CarDisplay;
