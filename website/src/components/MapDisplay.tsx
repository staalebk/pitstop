import { Alert, Slider, Text } from "@mantine/core";
import { useCallback, useEffect, useRef, useState } from "react";
import car_asset from "../assets/red_car_top_view.png";

type MapDisplayProps = {
  lat: number;
  lng: number;
  heading: number;
};

const defaultZoomLevel = 17;

const carImage = new Image();
carImage.src = car_asset;
carImage.style.width = "50px";
carImage.style.height = "50px";
carImage.style.position = "relative";
carImage.style.top = "25px";

const loadMaps = async (mapContainer: HTMLElement) => {
  const { Map } = (await google.maps.importLibrary(
    "maps",
  )) as google.maps.MapsLibrary;
  const { AdvancedMarkerElement } = (await google.maps.importLibrary(
    "marker",
  )) as google.maps.MarkerLibrary;

  // The map, centered at Uluru
  const mapObject = new Map(mapContainer, {
    zoom: defaultZoomLevel,
    center: { lat: 59, lng: 10 },
    zoomControl: false,
    disableDefaultUI: true,
    mapId: "DEMO_MAP_ID",
  });

  // The marker, positioned at Uluru
  const markerObject = new AdvancedMarkerElement({
    map: mapObject,
    position: { lat: 59, lng: 10 },
    content: carImage,
    title: "Car",
  });

  markerObject.style.transitionProperty = "top, left";
  markerObject.style.transitionDuration = "0.1s";

  return { mapObject, markerObject };
};

const MapDisplay = ({ lat, lng, heading }: MapDisplayProps) => {
  const [zoomLevel, setZoomLevel] = useState(defaultZoomLevel);
  const map = useRef<google.maps.Map | null>(null);
  const marker = useRef<google.maps.marker.AdvancedMarkerElement | null>(null);

  const containerStyle = {
    width: "100%",
    height: "200px",
  };

  const center = { lat, lng };

  const handleContainerAvailable = useCallback((container: HTMLDivElement) => {
    if (map.current === null) {
      loadMaps(container).then(({ mapObject, markerObject }) => {
        map.current = mapObject;
        marker.current = markerObject;
      });
    }
  }, []);

  useEffect(() => {
    if (map.current !== null && marker.current !== null) {
      map.current.panTo(center);
      carImage.style.rotate = `${heading}deg`;
      marker.current.position = center;
    }
  }, [center, heading]);

  useEffect(() => {
    if (map.current !== null) {
      map.current.setZoom(zoomLevel);
      carImage.style.width = `${15 + zoomLevel * 2}px`;
      carImage.style.height = `${15 + zoomLevel * 2}px`;
    }
  }, [zoomLevel]);

  return (
    <div>
      <Text>Position</Text>
      <div
        id="map-container"
        ref={handleContainerAvailable}
        style={containerStyle}
      >
        <Alert w="100%" h={200}>
          Map loading...
        </Alert>
      </div>
      <Text size="sm">
        Coordinates: {lat.toFixed(8)}, {lng.toFixed(8)}
      </Text>
      <Slider
        color="blue"
        step={2}
        min={5}
        max={22}
        value={zoomLevel}
        onChange={(value) => setZoomLevel(value)}
        marks={[
          { value: 5, label: "Space" },
          { value: 22, label: "Ant" },
        ]}
      />
    </div>
  );
};

export default MapDisplay;
