import { Alert, Text } from "@mantine/core";
import { GoogleMap, useJsApiLoader } from "@react-google-maps/api";
import { useCallback, useState } from "react";
import car_icon from "../assets/red_car_top_view.png";

type MapDisplayProps = {
  lat: number;
  lng: number;
};

const carIconImage = document.createElement("img");
carIconImage.src = car_icon;

let carMarker: google.maps.marker.AdvancedMarkerElement | undefined;

const MapDisplay = ({ lat, lng }: MapDisplayProps) => {
  const { isLoaded } = useJsApiLoader({
    id: "google-map-script",
    googleMapsApiKey: "AIzaSyCuuA86YeBqeHkD61-El-8U7_A3xwz2mOo",
  });
  const [, setMap] = useState<google.maps.Map | null>(null);

  const containerStyle = {
    width: "400px",
    height: "200px",
  };

  const center = { lat, lng };

  const onLoad = useCallback(
    (map: google.maps.Map) => {
      if (!isLoaded) {
        return;
      }

      if (typeof carMarker === "undefined") {
        carMarker = new google.maps.marker.AdvancedMarkerElement({
          map,
          position: center,
          content: carIconImage,
        });
      } else {
        carMarker.position = center;
      }

      setMap(map);
    },
    [center, isLoaded],
  );

  const onUnmount = useCallback(() => {
    setMap(null);
  }, []);

  if (!isLoaded) {
    return (
      <div>
        <Text>Position</Text>
        <Alert w={200} h={200}>
          Map loading...
        </Alert>
      </div>
    );
  }

  return (
    <div>
      <Text>Position</Text>
      <GoogleMap
        mapContainerStyle={containerStyle}
        center={center}
        options={{
          // Disable default UI
          disableDefaultUI: true,
          // Zoom control
          zoomControl: true,
        }}
        zoom={10}
        onLoad={onLoad}
        onUnmount={onUnmount}
      />
    </div>
  );
};

export default MapDisplay;
