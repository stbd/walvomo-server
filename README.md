# Walvomo

## What is this

Walvomo was/is a tool for visualizing votes of Finnish parliament. See video below (FIXME).

Walvomo was a hobby project that got started when I was a student at a University in Finland. The goal of one of the last courses was (slightly simplified): do some web stuff and then write a report about it. So, that's how it got started. Since it was interesting I kept working on the project after the course for some time. 

## Tech

Being most familiar with C and C++ languages, I wanted to try to use tools based on either one for the project. That's how the project ended up using Wt (kind of like Qt but for web instead of desktop) for server. I also thought that it was good opportunity to test key-value databases as they were hip at the time (and of course still are), that's how CouchBase ended up as the choice for database. And since stuff in CouchBase is just bytes, I used Protocol Buffers to serialize data. Tests use robot framework.

## Some notes about design

Project is split into two repositories; w_server (this repo) includes all C++ server things, w_data includes various scripts that I used to download and process data.

Since this is C++ I tried to keep efficiency in mind, that can be seen in things like I tried to allocate memory in advance. And for example load html templates only once. Not that that it was really necery not wise in all sittuations, at its peak site had 0.5 users per day, it handle those magnificently fast:)

In all cases, do note that this was a hobby project, it's not perfect, especially the scripts were bit ad-hoc.

## Why is this here

This is here mainly as a proof that I have, at some point of time, written some code, that does pretty much what it was meant to do, it is not perfect but I think it is fairly clean. Also, I guess theoretically it is possible that someone might get some value from this project.

## What's missing

I removed some icons to aboid binary in Git repo, and also removed OAuth settings from config files.
