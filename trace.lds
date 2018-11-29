
SECTIONS
{
	.text : {
	       traced_events_start = .;
	       *(.traced_events)
	       traced_events_end = .;
	}
}	
