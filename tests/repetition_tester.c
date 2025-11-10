typedef enum {
	REPETITION_TESTER_STATE_INITIALIZED,
	REPETITION_TESTER_STATE_TESTING,
	REPETITION_TESTER_STATE_ERROR,
	REPETITION_TESTER_STATE_COUNT
} repetition_tester_state;

typedef struct {
	repetition_tester_state state;

	u64 bytes_processed;

	u64 timer_start_tsc;
	u64 timer_end_tsc;
	i64 timer_start_page_faults;
	i64 timer_end_page_faults;
	
	u32 timer_start_count;
	u32 timer_stop_count;

	u64 test_max_tsc;
	u64 test_min_tsc;
	u64 test_min_page_faults;
	u64 test_max_page_faults;

	u32 test_count;
	u64 test_start_tsc;
	u64 test_length_tsc;
	u64 test_total_tsc;
	i64 test_total_page_faults;
} repetition_tester;

void repetition_tester_start_timing(repetition_tester *t)
{
	t->timer_start_count++;
	t->timer_start_page_faults = read_page_fault_count();
	t->timer_start_tsc = read_cpu_timer();
}

void repetition_tester_stop_timing(repetition_tester *t)
{
	t->timer_end_tsc = read_cpu_timer();
	t->timer_end_page_faults = read_page_fault_count();
	t->timer_stop_count++;
}

void repetition_tester_count_bytes(repetition_tester *t, u64 bytes_read)
{
	t->bytes_processed = bytes_read;
}

void repetition_tester_error(repetition_tester *t, const char *msg)
{
	t->state = REPETITION_TESTER_STATE_ERROR;
	log_error("REPETITION TESTER ERROR: %s", msg);
}

repetition_tester repetition_tester_create(u64 test_length_seconds)
{
	repetition_tester result = {0};
	result.state = REPETITION_TESTER_STATE_INITIALIZED;
	result.test_length_tsc = test_length_seconds * read_cpu_frequency();
	result.test_start_tsc = read_cpu_timer();
	return result;
}

b32 repetition_tester_is_testing(repetition_tester *t)
{
	switch(t->state)
	{
		case REPETITION_TESTER_STATE_ERROR:
		{
			return(false);
		} break;
		case REPETITION_TESTER_STATE_INITIALIZED:
		{
			t->state = REPETITION_TESTER_STATE_TESTING;
			return(true);
		} break;
		case REPETITION_TESTER_STATE_TESTING:
		{
			u64 elapsed = read_cpu_timer() - (t->test_start_tsc);
			u64 cpu_frequency = read_cpu_frequency();
			if(t->timer_start_count != t->timer_stop_count)
			{
				log_debug("%u (start count), %u (stop count)", 
					t->timer_start_count, t->timer_stop_count);
				log_error("REPETITION TESTER ERROR: test ran with uneven timer start count/timer" 
						  "stop count");
				return(false);
			}

			u64 timer_elapsed = t->timer_end_tsc - t->timer_start_tsc;
			i64 page_faults = t->timer_end_page_faults - t->timer_start_page_faults;
			if(t->test_min_tsc == 0)
			{
				_assert(t->bytes_processed != 0);
				t->test_min_tsc = timer_elapsed;
				t->test_min_page_faults = page_faults;
				t->test_max_tsc = timer_elapsed;
				t->test_max_page_faults = page_faults;
				t->test_start_tsc = read_cpu_timer();
				u64 bytes = t->bytes_processed;
				u64 cpu_frequency = read_cpu_frequency();
				f64 seconds = (f64)t->test_min_tsc / (f64)cpu_frequency;
				printf("\rmin: %llu, %.2lfms, %.3lfgb/s", t->test_min_tsc, seconds * 1000.0,
					(f64)(bytes / (1024 * 1024 * 1024)) / seconds);
				if(page_faults)
				{
					printf(", PFs: %.4lf (%.4lfkb/fault)", 
						(f64)page_faults, ((f64)t->bytes_processed/(f64)1024.0)/(f64)page_faults); 
				}
				else
				{
					printf("                                                                                                   ");
				}
				fflush(stdout);
			}
			else if(timer_elapsed < t->test_min_tsc)
			{
				_assert(t->bytes_processed != 0);
				t->test_min_tsc = timer_elapsed;
				t->test_min_page_faults = page_faults;
				t->test_start_tsc = read_cpu_timer();
				u64 bytes = t->bytes_processed;
				u64 cpu_frequency = read_cpu_frequency();
				f64 seconds = (f64)t->test_min_tsc / (f64)cpu_frequency;
				printf("\rmin: %llu, %.2lfms, %.3lfgb/s", t->test_min_tsc, seconds * 1000.0,
					(f64)(bytes / (1024 * 1024 * 1024)) / seconds);
				if(page_faults)
				{
					printf(", PFs: %.4lf (%.4lfkb/fault)", 
						(f64)page_faults, ((f64)t->bytes_processed/(f64)1024.0)/(f64)page_faults); 
				}
				else
				{
					printf("                                                                                                   ");
				}	
				fflush(stdout);
			}
			else if(timer_elapsed > t->test_max_tsc)
			{
				t->test_max_tsc = timer_elapsed;
				t->test_max_page_faults = page_faults;
			}

			t->test_total_tsc += timer_elapsed;
			t->test_total_page_faults += page_faults;
			t->test_count++;

			/* TODO: the printing stuff is a mess */
			if(elapsed > t->test_length_tsc)
			{
				_assert(t->test_count);
				u64 bytes = t->bytes_processed;
				u64 average = t->test_total_tsc / t->test_count;
				i64 average_page_faults = t->test_total_page_faults / (i64)t->test_count;
				f64 seconds = (f64)t->test_min_tsc / (f64)cpu_frequency;
				printf("\rmin: %llu, %.2lfms, %.3lfgb/s", t->test_min_tsc, seconds * 1000.0,
					(f64)(bytes / (1024 * 1024 * 1024)) / seconds);
				if(t->test_min_page_faults)
				{
					printf(", PFs: %.4lf (%.4lfkb/fault)", 
						(f64)t->test_min_page_faults, 
						((f64)t->bytes_processed/(f64)1024.0)/(f64)t->test_min_page_faults); 
				}
				else
				{
					printf("                                                                                                   ");
				}
				printf("\n");
				seconds = (f64)t->test_max_tsc / (f64)cpu_frequency;
				printf("max: %llu, %.2lfms, %.3lfgb/s", t->test_max_tsc, seconds * 1000.0,
					(f64)(bytes / (1024 * 1024 * 1024)) / seconds);
				if(t->test_max_page_faults)
				{
					printf(", PFs: %.4lf (%.4lfkb/fault)", 
						(f64)t->test_max_page_faults, 
						((f64)t->bytes_processed/(f64)1024.0)/(f64)t->test_max_page_faults); 
				}
				else
				{
					printf("                                                                                                   ");
				}
				printf("\n");
				seconds = (f64)average / (f64)cpu_frequency;
				printf("average: %llu, %.2lfms, %.3lfgb/s", average, seconds * 1000.0, 
					(f64)(bytes / (1024 * 1024 * 1024)) / seconds);
				if(average_page_faults)
				{
					printf(", PFs: %.4lf (%.4lfkb/fault)", 
						(f64)average_page_faults, 
						((f64)t->bytes_processed/(f64)1024.0)/(f64)average_page_faults); 
				}
				else
				{
					printf("                                                                                                   ");
				}
				printf("\n");
				return(false);
			}
			return(true);
		} break;
		default:
		{
			_assert(0);
			return(false);
		} break;
	}
}
