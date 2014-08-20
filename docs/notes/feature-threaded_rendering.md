# Threaded Rendering

In an effort to boost graphic rendering performance, when LiveCode is run on a machine with multiple cores, all stacks with accelerated rendering turned off will be split into multiple tiles, with each tile being rendered individually (and ideally concurrently). The number of tiles the stack is split into depends upon the machine the stack is running on.

So, for example, on a machine with 4 or more cores, a stack will be split up into 4 rectangular tiles - top left, top right, bottom left, bottom right. (On dual core machines, we use a top half bottom half split). Each tile will be rendered individually, meaning that in an ideal situation, we have 4 concurrent drawing operations providing a 4 fold speed increase. This is the ideal, but in reality, in the above case, we see closer to a 2 fold improvement. This will obviously vary with the number of concurrent operations the given machine can perform.

**Note that this feature is currently disabled on Windows due to threading conflicts caused by the player object.**