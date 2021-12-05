
import { useEffect, useState } from 'react';
const BASE_URL = "http://10.0.0.120/api/"

export function SupercarInfo() {

  let [supercar, setSupercar] = useState({})

  useEffect(
    () => {
      async function fetchData () {
        try{
          const result = await fetch(BASE_URL +'supercar');
          if(!result.ok){
            console.error("The response has an error", result.status)
            return
          }
          const json = await result.json();

          setSupercar(json); // asynchronously reset your form values
        }catch(e){
          console.error("Unable to get supercar configuration")
        }
    }
    fetchData()
  }, [supercar])

  return (

    <div>
      {supercar.power ? "ON" : "OFF"}
    </div>
  );
}