import React from 'react';
import ReactDOM from 'react-dom';
import './index.css';
import App from './App';
import reportWebVitals from './reportWebVitals';
import { BrowserRouter, Route, Routes } from 'react-router-dom';
import { SupercarConfigForm } from './routes/main_config';
import { MotorConfigForm } from './routes/motor_config';
import { SupercarInfo } from './routes/main_info';

ReactDOM.render(
  <React.StrictMode>
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<App />} >
          <Route
                index
                element={
                  <SupercarInfo />
                }
              />
          <Route path="main_config" element={<SupercarConfigForm />} />
          <Route path="motor_config" >
            <Route
              index
              element={
                <main style={{ padding: "1rem" }}>
                  <p>Select a type</p>
                </main>
              }
            />
            <Route path=":type" element={<MotorConfigForm />}/>
          </Route>
        </Route>
      </Routes>
    </BrowserRouter>
  </React.StrictMode>,
  document.getElementById('root')
);

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals();
