#all: 00-prod-cons 01-prod-cons 02-prod-cons 03-prod-cons 04-prod-cons \
	05-prod-cons 06-prod-cons 07-prod-cons 08-prod-cons

all: prod-cons

prod-cons: prod-cons.c queue_a.c queue_a.h
	gcc -g prod-cons.c queue_a.c -lpthread -o prod-cons

clean:
	rm -f prod-cons 

rmall:
	rmall '0?-prod-cons'
