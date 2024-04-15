import { Button, Center } from "@mantine/core";
import { PAGE } from "../commonTypes.ts";

type InitPanelProps = {
  setPage: (page: PAGE) => void;
};

const InitPanel = ({ setPage }: InitPanelProps) => {
  return (
    <Center h="calc(100dvh - 100px)">
      <Button onClick={() => setPage(PAGE.RACE_VIEW)}>
        Connect to Live Data
      </Button>
    </Center>
  );
};

export default InitPanel;
