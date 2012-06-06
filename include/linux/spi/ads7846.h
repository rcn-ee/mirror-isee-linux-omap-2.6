/* linux/spi/ads7846.h */

/* Touchscreen characteristics vary between boards and models.  The
 * platform_data for the device's "struct device" holds this information.
 *
 * It's OK if the min/max values are zero.
 */
enum ads7846_filter {
	ADS7846_FILTER_OK,
	ADS7846_FILTER_REPEAT,
	ADS7846_FILTER_IGNORE,
};

struct ads7846_platform_data {
	u16	model;			/* 7843, 7845, 7846, 7873. */
	u16	vref_delay_usecs;	/* 0 for external vref; etc */
	u16	vref_mv;		/* external vref value, milliVolts */
	bool	keep_vref_on;		/* set to keep vref on for differential
					 * measurements as well */
	bool	swap_xy;		/* swap x and y axes */

	/* Settling time of the analog signals; a function of Vcc and the
	 * capacitance on the X/Y drivers.  If set to non-zero, two samples
	 * are taken with settle_delay us apart, and the second one is used.
	 * ~150 uSec with 0.01uF caps.
	 */
	u16	settle_delay_usecs;

	/* If set to non-zero, after samples are taken this delay is applied
	 * and penirq is rechecked, to help avoid false events.  This value
	 * is affected by the material used to build the touch layer.
	 */
	u16	penirq_recheck_delay_usecs;

	u16	x_plate_ohms;
	u16	y_plate_ohms;

	u16	x_min, x_max;
	u16	y_min, y_max;
	u16	pressure_min, pressure_max;

	u16	debounce_max;		/* max number of additional readings
					 * per sample */
	u16	debounce_tol;		/* tolerance used for filtering */
	u16	debounce_rep;		/* additional consecutive good readings
					 * required after the first two */
	int	gpio_pendown;		/* the GPIO used to decide the pendown
					 * state if get_pendown_state == NULL
					 */
	int	(*get_pendown_state)(void);
	int	(*filter_init)	(const struct ads7846_platform_data *pdata,
				 void **filter_data);
	int	(*filter)	(void *filter_data, int data_idx, int *val);
	void	(*filter_cleanup)(void *filter_data);
	void	(*wait_for_sync)(void);
	bool	wakeup;
	unsigned long irq_flags;
};

struct ads7846 {
	struct input_dev	*input;
	char			phys[32];
	char			name[32];

	struct spi_device	*spi;
	struct regulator	*reg;

#if defined(CONFIG_HWMON) || defined(CONFIG_HWMON_MODULE)
	struct attribute_group	*attr_group;
	struct device		*hwmon;
#endif

	u16			model;
	u16			vref_mv;
	u16			vref_delay_usecs;
	u16			x_plate_ohms;
	u16			pressure_max;

	bool			swap_xy;

	struct ads7846_packet	*packet;
	struct ads7846_platform_data    *pdata;

	struct spi_transfer	xfer[18];
	struct spi_message	msg[5];
	int			msg_count;
	wait_queue_head_t	wait;

	bool			pendown;

	int			read_cnt;
	int			read_rep;
	int			last_read;

	u16			debounce_max;
	u16			debounce_tol;
	u16			debounce_rep;

	u16			penirq_recheck_delay_usecs;

	struct mutex		lock;
	bool			stopped;	/* P: lock */
	bool			disabled;	/* P: lock */
	bool			suspended;	/* P: lock */

	int			(*filter)(void *data, int data_idx, int *val);
	void			*filter_data;
	void			(*filter_cleanup)(void *data);
	int			(*get_pendown_state)(void);
	int			gpio_pendown;

	void			(*wait_for_sync)(void);
};
