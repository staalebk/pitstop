export enum PAGE {
  INIT,
  RACE_VIEW,
}

export type IncomingPacket = {
  uuid: string;
  vehicle: CarStateType;
};

export type RaceStateType = Record<string, CarStateType>;

export type CarStateType = {
  accelerator: number;
  brake: number;
  brake_temp: SixteenNumberArray;
  clutch: number;
  coolant_temp: number;
  heading: number;
  heart_rate: number;
  latitude: number;
  longitude: number;
  oil_temp: number;
  rpm: number;
  speed: number;
  timestamp: number; // Unix timestamp in microseconds
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
