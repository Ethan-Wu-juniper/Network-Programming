all:
	g++ sender_select.cpp -o sender_select
	g++ sender_sigalrm.cpp -o sender_sigalrm
	g++ sender_sockopt.cpp -o sender_sockopt
	g++ receiver.cpp -o receiver_select
	g++ receiver.cpp -o receiver_sigalrm
	g++ receiver.cpp -o receiver_sockopt

.PHONY: clean
clean:
	rm -f sender_select receiver_select sender_sigalrm receiver_sigalrm sender_sockopt receiver_sockopt