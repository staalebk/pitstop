export enum PAGE {
  INIT,
  RACE_VIEW,
}

export type RaceStateType = CarStateType[];

export type CarStateType = {
  id: string;
  timestamp: number;
  speed: number;
  heartrate: number;
  heading: number;
  brake_temp: SixteenNumberArray;
  rpm: number;
  position: [number, number];
};

export type SixteenNumberArray = [
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
  number,
];
