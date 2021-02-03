# Big data playground: Hadoop + Hive + Spark

[![Docker Build Status](https://img.shields.io/docker/cloud/build/panovvv/hadoop-hive-spark.svg)](https://cloud.docker.com/repository/docker/panovvv/hadoop-hive-spark/builds)
[![Docker Pulls](https://img.shields.io/docker/pulls/panovvv/hadoop-hive-spark.svg)](https://hub.docker.com/r/panovvv/hadoop-hive-spark)
[![Docker Stars](https://img.shields.io/docker/stars/panovvv/hadoop-hive-spark.svg)](https://hub.docker.com/r/panovvv/hadoop-hive-spark)

Base Docker image with just essentials: Hadoop, Hive and Spark.

## Software

* [Hadoop 3.2.0](http://hadoop.apache.org/docs/r3.2.0/) in Fully Distributed (Multi-node) Mode

* [Hive 3.1.2](http://hive.apache.org/) with HiveServer2 exposed to host.

* [Spark 2.4.5](https://spark.apache.org/docs/2.4.5/) in YARN mode (Spark Scala, PySpark and SparkR)

## Usage

Take a look [at this repo](https://github.com/panovvv/bigdata-docker-compose)
to see how I use it as a part of a Docker Compose cluster.

Hive JDBC port is exposed to host:
* URI: `jdbc:hive2://localhost:10000`
* Driver: `org.apache.hive.jdbc.HiveDriver` (org.apache.hive:hive-jdbc:3.1.2)
* User and password: unused.

## Version compatibility notes
* Hadoop 3.2.1 and Hive 3.1.2 are incompatible due to Guava version
mismatch (Hadoop: Guava 27.0, Hive: Guava 19.0). Hive fails with
`java.lang.NoSuchMethodError: com.google.common.base.Preconditions.checkArgument(ZLjava/lang/String;Ljava/lang/Object;)`
* Spark 2.4.4 can not 
[use Hive higher than 1.2.2 as a SparkSQL engine](https://spark.apache.org/docs/2.4.4/sql-data-sources-hive-tables.html)
because of this bug: [Spark need to support reading data from Hive 2.0.0 metastore](https://issues.apache.org/jira/browse/SPARK-13446)
and associated issue [Dealing with TimeVars removed in Hive 2.x](https://issues.apache.org/jira/browse/SPARK-27349).
Trying to make it happen results in this exception:
`java.lang.NoSuchFieldError: HIVE_STATS_JDBC_TIMEOUT`.
When this is fixed in Spark 3.0, it will be able to use Hive as a
backend for SparkSQL. Alternatively you can try to downgrade Hive :)

## Maintaining

* Docker file code linting:  `docker run --rm -i hadolint/hadolint < Dockerfile`
* [To trim the fat from Docker image](https://github.com/wagoodman/dive)

## TODO
* Upgrade spark to 3.0
* When upgraded, enable Spark-Hive integration.