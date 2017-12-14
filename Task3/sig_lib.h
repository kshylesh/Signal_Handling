
 

extern void sig_lib_init(void);


extern void sig_lib_clean_up(void);


extern int sig_handler_reg(pthread_t t_id);


extern int sig_handler_unreg(pthread_t t_id);
