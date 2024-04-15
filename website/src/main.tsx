import React from "react";
import ReactDOM from "react-dom/client";
import App from "./App.tsx";

const Main = () => (
  <React.StrictMode>
    <App />
  </React.StrictMode>
);

ReactDOM.createRoot(document.getElementById("root")!).render(<Main />);
