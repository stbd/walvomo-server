# Walvomo

## What is this

Walvomo was/is a tool for visualizing votes of Finnish parliament. See video below.

![walvomo](https://cloud.githubusercontent.com/assets/10348046/5587624/d024bfea-90f8-11e4-9f8d-e8586a307ca3.gif)

Walvomo was a hobby project that got started when I was a student at a University in Finland. The goal of one of the last courses was (slightly simplified): do some web stuff and then write a report about it. So, that's how it got started. And since it was interesting I kept working on it after the course for some time. 

## Tech

Being most familiar with C and C++ languages, I wanted to try to create web service using tools based on either language. That is how the project ended up using Wt (which is kind of like Qt but for web instead of desktop). I also thought that it was good opportunity to test key-value databases as they were hip at the time (and of course still are), that's how Couchbase ended up as the choice for database. And since stuff in Couchbase is just bytes, I used Protocol Buffers to serialize data. Tests use robot framework, and SCons is used for build.

## Some notes about design

Project is split into two repositories; walvomo-server (this repo) includes all C++ server things, walvomo-data includes various scripts that I used to download and process data. (TODO: create repository and link to it)

Since this is C++ I tried to keep efficiency in mind, that can be seen in things like I tried to allocate memory in advance. And for example load html templates to memory. Not that that it was really needed nor wise in all cases, at its peak the site had 0.5 users per day, it handled those magnificently fast:)

In all cases, do note that this was a hobby project mainly for learning, it's not perfect, especially the scripts were bit ad-hoc, and it was also the first encounter of CSS/HTML for me so those are far from perfect.

## Why is this here

This is here mainly as a proof that I have, at some point of time, written some code, that does pretty much what it was meant to do, it is not perfect but I think it is fairly clean. Also, I guess theoretically it is possible that someone might get some value from this project.

## What's missing

I removed some icons to avoid binary in Git repo, and also removed OAuth settings from config files.
