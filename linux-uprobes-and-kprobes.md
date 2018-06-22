# Linux kprobes (kernel probes) and uprobes (user-space function probes)

The linux kernel can hook a probe into every function entry and exit, a capability referred to _kprobes_. 
There is a similar capability for Linux user-space programs called _uprobes_. _kprobes_ and _uprobes_ are very low-level capabilities that do not by themselves provide probing capability -- they need a set of functions built around them.

## BCC
[bcc](https://github.com/iovisor/bcc) is just such a set of functions, and it has the advantage of embedding kprobe and uprobe programs within python command-line programs. The python programs are, as you would expect, relatively easy to modify.

Advantages of bcc include:
* hundreds of useful pre-built tools that are simple to turn into savior probes
* Python library and framework for creating new probes
* Stable and well-tested code base

Disadvantages include:
* uses a recent version of the LLVM clang compiler, not something that is already available as a package for some less recent distributions.

## Approaches for using bcc to create savior probes

There are two basic ways to create probes using bcc tools:
1. Invoke the particular bcc tool as a command-line utility, capture the output on stdio, enhance the output so it is consumable by the savior logging infrastructure.
2. Enhance the particular bcc tool so that it is a more functional Python class that accepts input and produces json-ized output that is directly consumable by the savior logging infrastructure. And, as python class it is composable into other python programs.

### Current Approach: Enhance each tool to be an interactive Pythonn class
I am working with the second approach from above. So far it is easier to enhance each tool directly using python than it is to capture and upgrade its stdio output.

## Repository Logistics
[bcc](https://github.com/iovisor/bcc) is too large of a project to copy a few files into savior/control/kernel. Therefore I decided to create a bcc submodule under savior/control/kernel/bcc.

To make it easier to maintain the changes I am making to the bcc tools we will use as probes, I first forked the upstream repository into [twosixlabs/bcc](https://github.com/twosixlabs/bcc), and then created a submodule from the fork. Now I can make commits to bcc directly from the submodule in savior/control/kernel/. Then, when appropriate, I can send pull requests from twosixlabs/bcc to the upstream repository.

I'm not entirely happy with the complexity of using a fork _and_ a submodule. If anyone has a better idea I may have overlooked, I'm happy to change the way I'm working with the project. Despite the complexity, the process of making and commiting changes is working well.

