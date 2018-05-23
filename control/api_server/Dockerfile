FROM ubuntu:14.04.3

# Set the locale, otherwise elixir will complain later on
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# wget for convenience, use curl if you want to
RUN apt-get update
RUN apt-get -y -q install wget python3-pip telnet curl

# get node js working. why, oh why, does Phoenix need to use Node for building assets?
RUN curl -sL https://deb.nodesource.com/setup_10.x | sudo -E bash -
RUN apt-get install -y nodejs
RUN npm install -g brunch

# add erlang otp
RUN wget https://packages.erlang-solutions.com/erlang-solutions_1.0_all.deb
RUN dpkg -i erlang-solutions_1.0_all.deb
RUN apt-get update
RUN apt-get install -y -q esl-erlang elixir
COPY . /app
WORKDIR /app
RUN chmod ugo+x /app/run.sh
RUN mix local.hex --force
RUN mix local.rebar --force
RUN mix deps.get --only-prod
RUN mix compile

# Asset compilation
RUN brunch build

# we've got custom HTTP status code that Plug needs to pickup from our config
RUN mix deps.compile plug

# setup support tooling
RUN pip3 install -r ./tools/get_certificates_requirements.txt

# setup directory for certificates
RUN mkdir -p /app/certs


CMD ["bash", "/app/run.sh"]
