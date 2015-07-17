#include "pez2001_mixer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

struct razer_effect *effect_mix = NULL;

int effect_mix_update(struct razer_fx_render_node *render)
{
	int x,y;
	#ifdef USE_DEBUGGING
		printf(" (Mixer::Basic.%d ## opacity:%f / %d,%d)",render->id,render->opacity,render->input_frame_linked_uid,render->second_input_frame_linked_uid);
	#endif
	//render->opacity = 0.5f;
	for(x=0;x<22;x++)
		for(y=0;y<6;y++)
		{
			rgb_mix_into(&render->output_frame->rows[y].column[x],&render->input_frame->rows[y].column[x],&render->second_input_frame->rows[y].column[x],render->opacity);
			render->output_frame->update_mask |= 1<<y;
		}
	return(1);
}

struct razer_effect *effect_null = NULL;

int effect_null_update(struct razer_fx_render_node *render)
{
	#ifdef USE_DEBUGGING
		printf(" (Compute::Null.%d ## )",render->id);
	#endif
	return(1);
}

struct razer_effect *effect_wait = NULL;

int effect_wait_update(struct razer_fx_render_node *render)
{
	#ifdef USE_DEBUGGING
		printf(" (Compute::Wait_update.%d ## )",render->id);
	#endif
	return(1);
}

int effect_wait_key_event(struct razer_fx_render_node *render,int keycode,int pressed)
{
	#ifdef USE_DEBUGGING
		printf(" (Compute::Wait_event.%d ## )",render->id);
	#endif
	if(pressed)
		return(0);
	return(1);
}



struct razer_effect *effect_transition = NULL;

int effect_transition_update(struct razer_fx_render_node *render)
{
	int length_ms = daemon_get_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,0));
	//int count = daemon_get_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,1));
	int dir = daemon_get_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,1));
	//count+=dir;
	unsigned long start = render->start_ticks;
	unsigned long end = start + length_ms;
	unsigned long ticks_left = end - render->daemon->chroma->update_ms;
	float opacity = 0.0f;
	if(dir == 1)
		opacity = (float)ticks_left / (float)length_ms;
	else
		opacity = 1.0f - ((float)ticks_left / (float)length_ms);
	#ifdef USE_DEBUGGING
		printf(" (Compute::Transition.%d ## length_ms:%d,dir:%d,opacity:%f)",render->id,length_ms,dir,opacity);
	#endif
	if(end<render->daemon->chroma->update_ms)
	{
		if(dir == 1)
		{
			dir = -1;
			if(render->parent) //compute effects should only be added as sub so this should be always fine
				render->parent->opacity = 0.0f;
		}
		else
		{
			dir = 1;
			if(render->parent) //compute effects should only be added as sub so this should be always fine
				render->parent->opacity = 1.0f;
		}
		daemon_set_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,1),dir);	
		return(0);
	}
	if(render->parent) //compute effects should only be added as sub so this should be always fine
		render->parent->opacity = opacity;

	//if(count<=0 || count>=daemon_get_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,0)))
	//	dir=-dir;
	//daemon_set_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,0),length_ms);	
	daemon_set_parameter_int(daemon_get_parameter_by_index(render->effect->parameters,1),dir);	
	return(1);
}


#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

void fx_init(struct razer_daemon *daemon)
{
	struct razer_parameter *parameter = NULL;
	effect_transition = daemon_create_effect();
	effect_transition->update = effect_transition_update;
	effect_transition->name = "Slow Opacity Transition";
	effect_transition->description = "First compute only effect";
	effect_transition->fps = 20;
	effect_transition->class = 2;
	effect_transition->input_usage_mask = RAZER_EFFECT_NO_INPUT_USED;
	//parameter = daemon_create_parameter_int("End Counter","End of animation (Integer)",44);//TODO refactor to daemon_add_effect_parameter_int(effect,key,desc,value)
	//daemon_add_parameter(effect_transition->parameters,parameter);	
	parameter = daemon_create_parameter_int("Effect Length","Time effect lasts in ms(INT)",2000);
	daemon_add_parameter(effect_transition->parameters,parameter);	
	parameter = daemon_create_parameter_int("Effect Direction","Effect direction value(INT)",1);
	daemon_add_parameter(effect_transition->parameters,parameter);	

	int effect_transition_uid = daemon_register_effect(daemon,effect_transition);
	#ifdef USE_DEBUGGING
		printf("registered compute effect: %s (uid:%d)\n",effect_transition->name,effect_transition->id);
	#endif

	effect_mix = daemon_create_effect();
	effect_mix->update = effect_mix_update;
	effect_mix->name = "Default Mixer";
	effect_mix->description = "Standard effect mixer";
	effect_mix->fps = 20;
	effect_mix->class = 1;
	effect_mix->input_usage_mask = RAZER_EFFECT_FIRST_INPUT_USED | RAZER_EFFECT_SECOND_INPUT_USED;
	int effect_mix_uid = daemon_register_effect(daemon,effect_mix);
	#ifdef USE_DEBUGGING
		printf("registered mix effect: %s (uid:%d)\n",effect_mix->name,effect_mix->id);
	#endif

	effect_null = daemon_create_effect();
	effect_null->update = effect_null_update;
	effect_null->name = "Empty Compute Node";
	effect_null->description = "Does nothing";
	effect_null->fps = 1;
	effect_null->class = 1;
	effect_null->input_usage_mask = RAZER_EFFECT_NO_INPUT_USED;
	int effect_null_uid = daemon_register_effect(daemon,effect_null);
	#ifdef USE_DEBUGGING
		printf("registered compute effect: %s (uid:%d)\n",effect_null->name,effect_null->id);
	#endif

	effect_wait = daemon_create_effect();
	effect_wait->update = effect_wait_update;
	effect_wait->key_event = effect_wait_key_event;
	effect_wait->name = "Wait For Key Compute Node";
	effect_wait->description = "Waits for a key and returns 0 ,it does nothing else";
	effect_wait->fps = 1;
	effect_wait->class = 1;
	effect_wait->input_usage_mask = RAZER_EFFECT_NO_INPUT_USED;
	int effect_wait_uid = daemon_register_effect(daemon,effect_wait);
	#ifdef USE_DEBUGGING
		printf("registered compute effect: %s (uid:%d)\n",effect_wait->name,effect_wait->id);
	#endif


}

#pragma GCC diagnostic pop


void fx_shutdown(struct razer_daemon *daemon)
{
	daemon_unregister_effect(daemon,effect_transition);
	daemon_free_parameters(&effect_transition->parameters);
	daemon_free_effect(&effect_transition);

	daemon_unregister_effect(daemon,effect_mix);
	daemon_free_parameters(&effect_mix->parameters);
	daemon_free_effect(&effect_mix);

	daemon_unregister_effect(daemon,effect_null);
	daemon_free_parameters(&effect_null->parameters);
	daemon_free_effect(&effect_null);

	daemon_unregister_effect(daemon,effect_wait);
	daemon_free_parameters(&effect_wait->parameters);
	daemon_free_effect(&effect_wait);
}