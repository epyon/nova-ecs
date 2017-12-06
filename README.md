# nova-ecs
Copyright (c) 2017 ChaosForge Ltd
http://chaosforge.org/

Simplified (and butchered!) version of the Nova ECS.

THIS IS NOT PRODUCTION READY CODE!

It has been stripped down from the ECS implementation in Nova, has not been tested, and serves for educational purposes only. Some of the design decisions might not make sense in this stripped down version, but they're a consequence of the full system. In many places in the code we use STL inefficiently, due to the fact that for the purposes of keeping this sample small, we substituted all Nova implementations of algorithms and data structures with STL ones. That said, if you want to submit patches for correctness (not increasing the complexity of this example code), please do so!

test.cc has some sample code, but it barely scratches the possibilities of the ECS implementation.

Nova is the engine that powers Jupiter Hell ( http://jupiterhell.com ). The full ECS implementation, as well as the engine itself, might be open sourced some day, but no sooner than the full release of Jupiter Hell.

If you're interested in my work, follow me at twitter ( https://twitter.com/epyoncf ). I'm also doing live development of Jupiter Hell on Twitch - https://www.twitch.tv/epyoncf

regards,
Kornel Kisielewicz
