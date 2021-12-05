import { useForm } from "react-hook-form";
import { useEffect } from 'react';
const BASE_URL = "http://10.0.0.120/api/"

export function SupercarConfigForm() {
    const { register, handleSubmit, reset} = useForm();
  
  const onSubmit = async (data) => {
    try{
      const result = await fetch(BASE_URL + "supercar/config", {
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
          const result = await fetch(BASE_URL +'supercar/config');
          if(!result.ok){
            console.error("The response has an error", result.status)
            return
          }
          const json = await result.json();

          reset(json); // asynchronously reset your form values
        }catch(e){
          console.error("Unable to get supercar configuration")
        }
    }
    fetchData()
  }, [reset])

  
  return (

    <form onSubmit={handleSubmit(onSubmit)}>
      <fieldset>
        <label>Maximum speed</label> 
        <input {...register("max_speed", { required: true, valueAsNumber: true, min: 0, max: 100 })} />
        <label>Delta speed (Speed limit increment)</label> 
        <input {...register("delta_speed", { required: true,  valueAsNumber: true, min: 0, max: 100 })} />
        <label>Front distance threshold</label>
        <input {...register("distance_threshold_forward", { required: true, valueAsNumber: true, min: 0, max: 25 })} />
        <label>Back distance threshold</label>
        <input {...register("distance_threshold_backward", { required: true, valueAsNumber: true, min: 0, max: 25 })} />    
      </fieldset>
      <fieldset>     
        <label>Mode input GPIO (Rocker switch)</label> 
        <input {...register("mode_input_pin", { required: true,  valueAsNumber: true, min: 0, max: 39 })} />
        <label>Mode output GPIO (Relay)</label>
        <input {...register("mode_output_pin", { required: true,  valueAsNumber: true, min: 0, max: 39 })} />
        <label>Power output GPIO (Relay)</label>
        <input {...register("power_output_pin", { required: true,  valueAsNumber: true, min: 0, max: 39 })} /> 
      </fieldset>
      <input type="submit" />
    </form>
  );
}