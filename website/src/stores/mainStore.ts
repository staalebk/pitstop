/*
type MainStoreType = {
  page: VISIBLE_PAGE;
  connectionState: ReadyState;
  race: RaceStateType;

  goToRaceView: () => void;
  goToInitPage: () => void;
};

const mainStore: MainStoreType = store({
  page: VISIBLE_PAGE.INIT,
  connectionState: ReadyState.CLOSED,
  race: [],

  goToRaceView: () => {
    console.log("Enabling race view and connecting to websocket...");
    mainStore.page = VISIBLE_PAGE.RACE_VIEW;
  },
  goToInitPage: () => {
    console.log("Back to start up page, disconnecting from websocket...");
    mainStore.page = VISIBLE_PAGE.INIT;
  },
});

autoEffect(() => {
  console.log("Connection state updated: ", mainStore.connectionState);
});

export default mainStore;
*/
