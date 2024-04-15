export enum PAGE {
  INIT,
  RACE_VIEW,
}

export type RaceStateType = CarStateType[];

export type CarStateType = {
  id: string;
  timestamp: number;
  speed: number;
  heading: number;
  brake_temp_left: SixteenNumberArray;
  brake_temp_right: SixteenNumberArray;
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
