#include <errno.h>
#include <stdio.h>
//stdlib.h头文件必须引入，要不然atof的转换结果会异常
#include <stdlib.h>
#include <pthread.h>
#include <modbus/modbus.h>
#include <mosquitto.h>

static bool connected = true;
modbus_t *modbus_ctx;
struct mosquitto *mosq_ctx;
static int mid_sent = 0;

void print_usage(void)
{
	printf("iiotdemo is a simple gateway demo.\n");
	printf("Usage: iiotdemo mqtt_host plc_ip plc_port.\n");
	printf("       iiotdemo 192.168.4.144 192.168.4.120 502.\n");

}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	//接收服务器下发修改设备PLC的控制温度指令
	if(message->payloadlen != 0)
	{
		uint16_t temp_reg[2];
		float *temp = (float *)temp_reg;
		//将字符串转换为float写入到PLC
		*temp = atof(message->payload);
		int rc = modbus_write_registers(modbus_ctx, 0, 2, temp_reg);
		if(rc == -1)
		{
			printf("%s\n", modbus_strerror(errno));
		}
	}
}


void my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	//订阅服务器下发的控制指令
	mosquitto_subscribe(mosq, NULL, "com.freeiris.iiot.control.iiotdemo-12345", 1);
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	connected = false;
}

void my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	
}

void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	//如果MQTT有异常，可以去掉下面的注释
	//printf("%s\n", str);
}

void plc_thread(void*param)
{
	int i;
	int rc;
	printf("plc_thread\n");
	while(connected)
	{
		//读者要注意，直接用下面的命令读取的时候40001相当于第0个通道
		//读取PLC当前温度
		uint16_t temp_reg[2];		
		rc = modbus_read_registers(modbus_ctx,0,2,temp_reg);
		if (rc == -1) {
			printf("%s\n", modbus_strerror(errno));
			return -1;
		}		
		float *temp = (uint32_t *)temp_reg;

		//读取PLC当前产量
		uint16_t yields_reg[2];
		rc = modbus_read_registers(modbus_ctx,2,2,yields_reg);
		if (rc == -1) {
			printf("%s\n", modbus_strerror(errno));
			return -1;
		}
		uint32_t *yields = (uint32_t *)yields_reg;

		char payload[256] = {0};
		//%.2f的意思是保留2位小数
		sprintf(payload,"iiotdemo-12345,%.2f,%ld",*temp,*yields);
		mosquitto_publish(mosq_ctx,&mid_sent,"com.freeiris.iiot.report",strlen(payload),payload,0,0);
		sleep(1);
	}
}

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		print_usage();
		return 1;
	}
	//定义modbus实例，连接到PLC
	modbus_ctx = modbus_new_tcp(argv[2], atoi(argv[3]));
	modbus_set_slave(modbus_ctx, 1);
	//连接到PLC，如果无法连通，直接报错返回1
	if (modbus_connect(modbus_ctx) == -1) {
		printf("can't connect to PLC!\n");
		return 1;
	}
	
	/*用设备名称作为MQTT的CientID，ClientID当做网关的唯一识别ID，本实例直接用12345，读者也可以尝试用GUID来解决重复问题*/
	char client_id[256] = {0};
	strcpy(client_id,"iiotdemo-12345");
	mosq_ctx = mosquitto_new(client_id, true, NULL);

	//如果失败，报错并返回
	if(!mosq_ctx){
		switch(errno){
			case ENOMEM:
				printf("Error: Out of memory.\n");
				break;
			case EINVAL:
				printf("Error: Invalid id.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}

	//设置MQTT实例的回调函数
	mosquitto_log_callback_set(mosq_ctx, my_log_callback);
	mosquitto_connect_callback_set(mosq_ctx, my_connect_callback);
	mosquitto_disconnect_callback_set(mosq_ctx, my_disconnect_callback);
	mosquitto_publish_callback_set(mosq_ctx, my_publish_callback);
	mosquitto_message_callback_set(mosq_ctx, my_message_callback);

	//连接到MQTT服务器
	int rc = mosquitto_connect(mosq_ctx, argv[1], 1883, 60);

	if(rc == 0)
	{
		pthread_t plc_thread_id = 0;
		pthread_create(&plc_thread_id,NULL,(void *) plc_thread,NULL); 
		//通过mosquitto_loop_forever函数，让MQTT保持连接状态，就可以收到消息代理下发的内容
		rc = mosquitto_loop_forever(mosq_ctx, -1, 1);
		printf("exit iiot demo..........................\n");
		mosquitto_destroy(mosq_ctx);
		mosquitto_lib_cleanup();
	}
	else
	{
		printf("can't connect to mqtt server!\n");
		return 1;
	}

	return 0;
}
