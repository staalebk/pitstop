import {
  AppShell,
  Button,
  Grid,
  MantineProvider,
  Text,
  Title,
} from "@mantine/core";
import "@mantine/core/styles.css";
import { useState } from "react";
import { ReadyState } from "react-use-websocket";
import { PAGE } from "./commonTypes.ts";
import InitPanel from "./components/InitPanel.tsx";
import RaceView from "./components/RaceView.tsx";
import StatusBadge from "./components/StatusBadge.tsx";

const App = () => {
  const [page, setPage] = useState<PAGE>(PAGE.INIT);
  const [time, setTime] = useState<number | null>(null);
  const [connectionState, setConnectionState] = useState<ReadyState>(
    ReadyState.CLOSED,
  );

  return (
    <MantineProvider>
      <AppShell header={{ height: 60 }} padding="md" ff="Inter">
        <AppShell.Header>
          <Grid justify="space-between" align="stretch">
            <Grid.Col span="content">
              <Title ms={10} mt={5}>
                Pitstop Client
              </Title>
            </Grid.Col>
            <Grid.Col span="content">
              <StatusBadge connectionState={connectionState} />
            </Grid.Col>
          </Grid>
        </AppShell.Header>
        <AppShell.Footer>
          {page === PAGE.RACE_VIEW && (
            <Grid justify="space-between">
              <Grid.Col span="content">
                <Text ms={10} mt={15}>
                  {time && `Last update: ${time}`}
                </Text>
              </Grid.Col>
              <Grid.Col span="content">
                <Button onClick={() => setPage(PAGE.INIT)} m={10}>
                  Disconnect
                </Button>
              </Grid.Col>
            </Grid>
          )}
        </AppShell.Footer>
        <AppShell.Main>
          {page === PAGE.INIT && <InitPanel setPage={setPage} />}
          {page === PAGE.RACE_VIEW && (
            <RaceView
              setPage={setPage}
              setConnectionState={setConnectionState}
              setTime={setTime}
            />
          )}
        </AppShell.Main>
      </AppShell>
    </MantineProvider>
  );
};

export default App;
