import './App.css';
import { Link, Outlet } from 'react-router-dom';

function App(){
  return (
  <div className="App">
    <header>
      <nav>
        <ul>
          <li>
            <Link  to="/">Home</Link>
          </li>
          <li>
            <Link to="/main_config">Main configuration</Link>
          </li>
          <li>
            <Link to="/motor_config/propulsion">Propulsion configuration</Link>
          </li>
          <li>
            <Link to="/motor_config/steering">Steering configuration</Link>
          </li>
        </ul>
      </nav>
    </header>
    <main>
     <Outlet />
    </main>
  </div>
  )
}



export default App;
 