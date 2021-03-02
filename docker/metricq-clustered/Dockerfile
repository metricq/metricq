FROM rabbitmq:management-alpine

ADD rabbitmq.conf /etc/rabbitmq/
ADD --chown=rabbitmq:rabbitmq .erlang.cookie /var/lib/rabbitmq/
RUN chmod 600 /var/lib/rabbitmq/.erlang.cookie
