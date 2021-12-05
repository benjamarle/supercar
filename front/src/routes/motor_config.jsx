import { useForm } from "react-hook-form";
import { useEffect } from 'react';
import { useParams } from "react-router";
const BASE_URL = "http://10.0.0.120/api/"

export function MotorConfigForm() {
  const { register, handleSubmit, reset} = useForm();

  const params = useParams();
  const url = BASE_URL + "supercar/"+params.type+"/config";
  
  const onSubmit = async (data) => {
    try{
      const result = await fetch(url, {
        method: 'PUT', 
        body: data
      })
      if(!result.ok){
        console.error("The submit of the form data returned an error", result.status)
      }
    }catch(e){
      console.error("Could not send form data", e);
    }
  }

  useEffect(
    () => {
      async function fetchData () {
        try{
          const result = await fetch(url);
          if(!result.ok){
            console.error("The response has an error", result.status)
            return
          }
          const json = await result.json();

          reset(json); // asynchronously reset your form values
        }catch(e){
          console.error("Unable to get supercar "+params.type+" configuration")
        }
    }
    fetchData()
  }, [reset, params])

  
  return (

    <form onSubmit={handleSubmit(onSubmit)}>
      <fieldset>
        <label>Acceleration (%)</label> 
        <input {...register("acceleration", { required: true, valueAsNumber: true, min: 0, max: 100 })} />
        <label>Control period (μs)</label> 
        <input {...register("ctrl_period", { required: true,  valueAsNumber: true, min: 0, max: 100 })} />
        <label>PWM Frequency (Hz)</label>
        <input {...register("pwm_freq", { required: true, valueAsNumber: true, min: 0, max: 20000 })} />
      </fieldset>
      <fieldset>     
        <label>PWM output GPIO</label> 
        <input {...register("pwm_pin", { required: true,  valueAsNumber: true, min: 0, max: 39 })} />
        <label>Direction output GPIO (Relay)</label>
        <input {...register("direction_pin", { required: true,  valueAsNumber: true, min: 0, max: 39 })} />
      </fieldset>
      <input type="submit" />
    </form>
  );
}