#import "preamble.typ": *

#set text(lang: "en")

#show: ilm.with(
  title: [Real-Time Graphics on\ the Xilinx Zynq FPGA],
  author: [Andreas Pedersen & Alexandre Cherencq],
  date: datetime(year: 2025, month: 01, day: 03),
  abstract: [
    Submission for the Assignment Project in the _Embedded Real-Time Systems_ course. A platform capable of real-time full HD graphics at 60 frames-per-second. Implemented using HW/SW co-design.\

    #text(blue)[
    #link("https://github.com/Ponti17/zybo-graphics")[GitHub (link)]
    ]

    #text(size: 9pt)[Department of ECE, Aarhus University]
  ],
  preface: [
    #align(center + horizon)[
      _This page intentionally left blank._
    ]
  ],
  appendix: (
    enabled: true,
    title: "Appendix A",
    body: [
        #v(200pt)
        #figure(
          rotate(-90deg, image("figures/bd_simple.png", width: 150%))
        )
    ]
  ),
  bibliography: bibliography("refs.bib", style: "ieee"),
  figure-index: (enabled: true),
  table-index: (enabled: true),
  listing-index: (enabled: true),
)

= Introduction

== Description
As embedded devices have gotten more and more powerful, it has become increasingly popular to use them for graphics user interfaces. Real-time graphics, however, is both computationally intensive and requires high memory bandwidth. In addition many displays require dedicated hardware to interface with.

In many real-world applications, graphics is *only* used as an user interface, meaning that the CPU must have time for other tasks. Even if the graphics are simple, memory bandwidth is often a limiting factor that will keep the CPU busy for long periods of time as it writes to frame buffers. 

A typical real-time graphics system uses _dual buffering_ i.e. two "display sized" arrays allocated in memory. As `frame[0]` is being rendered, `frame[1]` can be shown on the display. After vertical synchronization (V-sync) the buffers switch so `frame[0]` is shown while the next frame is rendered in `frame[1]`. For an ideal operation of 60 frames-per-second (FPS), a total of $1"/"60 = 16.6 "ms"$ are available between V-syncs. This effectively makes real-time graphics one of the most challenging real-time systems.

In this project we will detail the design and implementation of a complete graphics platform, complete with a dedicated _Graphics Processing Unit_ (GPU), capable of rendering independently of the CPU, and a display controller that handles the strict timing necessary to transmit video over HDMI. As a simple demonstration of the platform we chose to implement the classic two-player pong game.

#figure(
  image("figures/pong_fb_dump.png", width: 90%),
  caption: [Framebuffer dump of Pong running at 1920x1080\@60FPS.],
) <fb_dump>

= Methodology
For our project, the used methodology is strongly inspired by the UML-based Hardware/Software co-design methodology proposed in an article on how to use UML design for embedded systems@teknologiskInstitut. The described methodology is adapted to this project.

This methodology is iterative, and consists of three phases: 
- System analysis
- System design
- System implementation and validation

In the next section the sections, the objectives, goals, and tasks for each phase will be briefly explained, as well as the types of UML diagrams used in the development and progress of each. 

== System Analysis
In the analysis phase, the objective is to determine _what needs to be made_. To do so, a use-case diagram and description will be made to visualize the functional requirements of the system.
The different components of the system will be described on a higher level using domain class diagrams. This will help to better understand the relationship between the different components in the system. 
Upon having a class diagram representing the domain, this can be expanded for the application layer, giving a more detailed overview over the application. 

A sequence diagram will also be made to help understand the flow of interactions between the different components, and also to conceptualize their interfacing mechanisms. 

== System Design
In this phase the objective is to describe _how the system will be implemented_.
To achieve this, one must work on the architecture and the interfaces needed. In this phase, some of the considerations must also be put towards partitioning the system into hardware and software components. As described in the article @teknologiskInstitut, this is done by evaluating the chosen (or in this case given) platform, and by experience. Sadly, we are inexperienced, so this part might be subject for multiple iterations. 
  
As mentioned above, the focus is on how to do the implementation. Therefore, using the proposed methodology, four architectural views  will be developed, namely Process, logical, deployment and implementation. 

== System Implementation and Validation
Finally, the implementation and validation phase focuses on _making and verifying_ the system. This phase is the actual conception of the system, and also what might lead to the discoveries of issues with the current iterations' architecture. These potential issues, leads us back to the system analysis, and from there to the system design phase, forcing a refinement of the previous plans and ideas. 

#pagebreak()
= Theory

== Computer Graphics

In computer graphics images are represented by pixels. Pixels have a _pixel format_: a way to represent the color and transparency of the pixel with digital valuess. A common pixel format is ARGB, where each pixel has four channels: *A* (alpha), *R* (red), *G* (green) and *B* (blue). The last three channels describe the color, while the alpha channel encodes the *opacity* i.e. how transparent or opaque it is.

The more _bits-per-pixel_ (bpp), the more precise color representations are possible. As will be established later, bandwidth is almost entirely the only limiting performance factor, so fewer bits will more or less yield a linear increase in performance. The ARGB pixel format is often 32bpp, with 8 bits for each channel (sometimes written as ARGB8888). Other pixel formats such as RGB565 reduce the number of bits for each color and omit the alpha channel entirely. This leads to a high increase in performance but with a significant trade-off in color accuracy leading to _banding_ (#ref(<banding>)).

#figure(
  image("figures/rgb24_v_rgb565.svg", width: 100%),
  caption: [Noticable color banding when using pixel formats with less than 8 bits per channel.],
) <banding>

== Alpha Compositing

The process of combining two images (often called *foreground* and *background*) to create the appearance of partial (or full) transparency is called *alpha compositing*. The pixel values in the final image are determined by *blending* the images according to their alpha values.

One of the most commonly used alpha blending operators is the *over* operator. Denoting the foreground color $C_("fg")$ and the background color $C_("bg")$, and their corresponding alpha values $alpha_("fg")$ and $alpha_("bg")$. The resulting color $C_("out")$ after alpha compositing the foreground *over* the background is:

$ C_("out") = ((255 - alpha_("fg")) C_("bg") + alpha_("fg") C_("fg"))/255 $ <alpha_blending>

#ref(<alpha_blending>) is then applied to each color channel separately. Blending is computationally intensive and therefore a top priority for hardware acceleration.

#blockquote[
Note that we perform division in #ref(<alpha_blending>) leading to inevitable rounding errors on hardware with limited precision. To circumvent this we will use the larger `ap_uint<16>` type for intermediate operations in HLS.
]

#pagebreak()
== Real-Time Video

#v(-10pt)
#figure(
  image("figures/video_frame.svg", width: 105%),
  caption: [Example video frame #cite(<VTC>).],
) <video-timing>

All video systems require management of video timing signals, which are used to synchronize a variety of processes #cite(<VTC>). A video frame consists of active video and blanking periods. The vertical and horizontal synchronization signals (V-sync, H-sync) describe the video frame timing, which includes active and blanking data. A frame is drawn from the top and down, one row at a time.

- *V-Sync*: Indicates the start of a new video frame. It is important to synchronize the *entire* graphics system with this signal. At V-Sync a new framebuffer will be shown, and if the system is not done rendering, artifacts such as tearing will occur.

- *H-Sync*: Marks the start of a new horizontal line within a frame. It synchronizes the horizontal drawing process.

In addition to the synchronization signals we also have the *porch intervals*: _Horizontal Front Porch_ (HFP), _Vertical Back Porch_ (VBP). These intervals acts as "buffer zones" between the synchronization pulses and active video. Allowing the display hardware to prepare for the next line of video data (HFP) and the graphics pipeline to prepare before the start of the next frame (VBP).

#blockquote[
This seems like a lot to keep in mind, but in reality the _Video Timing Controller_ handles all of this. The VTC is simply configured with the timings for 1920x1080\@60FPS. The important point is that the system *must* synchronize with the V-Sync.
]

= Requirements
To specify the system we need to form functional and non-functional requirements. The non-functional requirements are verifiable features for the system. The functional requirements are described through use cases.

== Functional Requirements
Our _Zybo Console_ only has one use case, which is *old-school two-player pong*. Upon startup pong is loaded and with a total of four input buttons, two players can play against each other. The use case is described in #ref(<usecase>). An use-case diagram is shown in #ref(<usecase-diag>).

#figure(table(
  columns: (auto, 1fr),
  inset: 5pt,
  align: left,
  table.header([*Actor*], [*System User*]),
  [*Precondition*], [The Zybo PL is programmed and the software application is loaded.\
  A display is connected.],
  [*Postcondition*],  [One of the players won the game (or both got tired).],
  [*Main Path*], [
    1. System turns on and pong is displayed on screen.
    2. Game starts upon starting players input.
    3. A vicious game of pong occurs.
    4. A player has won.
    5. The game gives feedback to the winning player.
    6. Game returns to starting state.
  ],
),
  caption: [
    Zybo console use case.
  ]
) <usecase>

#figure(
  image("figures/system_analysis_use_case.svg", width: 100%),
  caption: [Use case diagram of two-player pong.]
) <usecase-diag>

#pagebreak()
== Non-functional requirements
Non-functional requirements have an impact on how the system must be implemented, and are also what puts strict requirements on our GPU IP. The requirements listed below are slightly ambitious but should be possible on a Zynq system.

- *[R1]* The system *must* be capable of real-time rendering at Full HD (1920x1080).

- *[R2]* The system *must* be capable of real-time rendering at 60 FPS.

- *[R3]* Video output over HDMI.

- *[R4]* No more than three frames of delay between user input and the action occurring on screen (approximately 50ms at 60FPS).

- *[R5]* The GPU *must* be capable of alpha compositing.

- *[R6]* The combined PL system must not exceed capabilities of the Zybo board.

#pagebreak()
= System Analysis

#figure(
  image("figures/domain_diagram.svg", width: 100%),
  caption: [Domain diagram of Zybo graphics platform.],
) <domain_diagram>

To model the system we start of with a high-level analysis. After analyzing our problem description, primary use case and requirements we identify what is necessary to realize the use case with the limitations from the non-functional requirements. See in #ref(<domain_diagram>) the domain diagram of our Zybo graphics platform.

The players will input using buttons. The CPU reads the input and calculates the game state. From the game state a _command list_ (CL) is created that contains a "recipe" for how to draw the scene. The GPU block uses the CL to render the scene in a framebuffer. Finally the framebuffer is displayed by the _Display Controller_ and observed by the players.

== System Behavior
The behavior of the top-level system is shown in the activity diagram (#ref(<activity_gpio>)). 

#figure(
  image("figures/activity_gpio.svg", width: 100%),
  caption: [Activity diagram of top-level system.],
) <activity_gpio>

= System Design

== Partitioning

#figure(
  image("figures/partitioning.svg", width: 100%),
  caption: [Deployment diagram of Zybo graphics platform.],
) <partitioning_diagram>

From the description and requirements we start specifying the implementation. In #ref(<partitioning_diagram>) we see how we _partition_ the system. The _Processing System_ (PS) handles player input and updates the game state and creates a CL. While in the _Programming Logic_ (PL) we implement a block design containing the Display Controller and GPU. Command lists and framebuffers will be located in external DDR memory.

== Process Diagram

A process diagram of the PS is shown in #ref(<process-diagram>). A description is shown in the table below.

#figure(table(
  columns: (0.2fr, 1fr),
  inset: 5pt,
  align: left,
  table.header([*Process*], [*Description*]),
  [`main()`], [The main process is the application starting point. All the peripherals we use are initialized and configured. After that the V-sync interrupt from the _Video Timing Controller_ is registered with the PS _Generic Interrupt Controller_. The main application task is registered with the FreeRTOS kernel, and finally the scheduler is started.],
  [`appTask()`], [The application task waits to receive a dummy message in a queue of depth 1. This acts as a synchronization point for the application and the V-sync signal. When a message is received in the queue, `appTask()` awakens and switches framebuffers, updates the game state and starts the GPU.],
  [`VTC ISR`], [The _Interrupt Service Routine_ (ISR) for the _Video Timing Controller_ is triggered on every V-sync. The ISR simply places a message in the queue, awakening `appTask()`.]
)
)

#figure(
  image("figures/process.svg", width: 100%),
  caption: [Process diagram of the processing system.]
) <process-diagram>

After updating the game `appTask()` goes back to idle, waiting to receive a new message in the queue. It is important to note that the idle state is flexible, and does not represent a state in which the CPU is not capable of processing. Using the RTOS scheduler another task can easily be registered and scheduled in when the CPU has time available. This way the system is expandable and completely avoids _busy waiting_.

== Rendering Pipeline

#figure(
  image("figures/render_pipeline.svg", width: 105%),
  caption: [Rendering pipeline utilizing parallel computing of the GPU and core.],
) <draw_cycle>

We designed our system to be _dual buffered_, i.e. we have two framebuffers. We then switch between presenting one and rendering the other. In addition we also have two command lists associated with their respective frame buffer.

The advantage of having an independent GPU is best understood by seeing the first three draw cycles upon startup (#ref(<draw_cycle>)). The CPU ("Core") calculates and uses the game state to prepare draw commands ("Prepare CL [0]"), the GPU executes those commands ("Render Frame [0]"), and the display eventually presents the frame ("Show Frame [0]"). See how the CPU and GPU work in parallel. While the GPU renders the current frame the CPU is already preparing the next. By keeping the CPU and GPU out of sync this way we increase the percentage of time spent computing/rendering. The only disadvantage is that the frame shown on screen will be delayed.

== Display Controller
#v(-10pt)
#figure(
  image("figures/video_pipeline.svg", width: 100%),
  caption: [Block definition diagram of the display controller.],
) <video-pipeline>

#figure(table(
  columns: (0.5fr, 1fr),
  inset: 5pt,
  align: left,
  table.header([*IP*], [*Purpose*]),
  [*Video DMA (VDMA)*], [Provides high-bandwidth asynchronous direct memory access to our framebuffers. Outputs the data as AXI stream.],
  [*Video Timing Controller (VTC)*], [Generates the necessary timing to output video.],
  [*Stream to Video Out*], [Converts AXI4-Stream signals to parallel RGB video output.],
  [*RGB2DVI*], [Encodes parallel RGB signals as _transition-minimized differential signaling_ (TMDS), used for HDMI.]
),
  caption: [
    IP cores used for real-time video out.
  ]
) <video_stream>

A block definition diagram of the video pipeline is shown in #ref(<video-pipeline>), and the IP cores used are summarized in #ref(<video_stream>). The _Video DMA_ reads the memory mapped framebuffer and outputs it as AXI-Stream. The _AXI-Stream to Video Out_ core converts the stream to parallel RGB. Finally the _RGB2DVI_ core encodes the parallel RGB signals as TMDS and outputs it over HDMI. To encode a parallel RGB 1920x1080\@60FPS signal as TMDS a 148.5MHz pixel clock is necessary, generated by the _dynclk_ core.

== Draw Cycle
In #ref(<sequence_diagram>) are shown the sequence diagram for a typical frame render, expanding on #ref(<draw_cycle>). Note that the diagram explicitly ignores whatever game or other graphics application might be running. We see the different signals required to operate the real-time graphics system. Each time the CPU has finished a new CL it _binds_ it and the active framebuffer to the GPU and starts rendering. Upon the next VSync the buffers are switched.

#figure(
  image("figures/sequence.svg", width: 110%),
  caption: [Sequence diagram of two draw cycles.],
  gap: -10pt
) <sequence_diagram>

== Game Engine Design<game-engine-design>
#figure(
  image("figures/class_diagram_engine.svg", width: 90%),
  caption: [Game engine class diagram.],
) <class-diagram>

To model a game engine that can run the game Pong, and also other games if wished, the classes in #ref(<class-diagram>) are identified as building blocks for basic games. Working in a two dimensional visual representation, a simple data structure Vec2 is identified enabling the use of 2D-vectors. Needing the ability to do arithmetics and logic comparisons, operator overloading will be implemented. Since the final rendering will take place in a discrete two-dimensional grid, a conversion method will also be needed if the Vec2 is used as a float. 

Games are managed using four different classes. The context class is responsible of managing the active game scene. In more complicated games, multiple scenes can be implemented using the state pattern @gangOfFour. It is also responsible of delegating a game state update as well as the render call to the active scene. 

The scene class is tasked with ownership of the different game objects used in its scene. Like the context class, it delegates the game state update and the render to the owned game objects. Inspired by the composite pattern @gangOfFour, the scene has a collection of objects, but they do not share the same interface. 

The game object class is an abstract class, letting the game designer choose how to implement the game state update. This allows for computing interactions between the different game objects. 

Lastly a shape class, Rectangle2D, is used to define the game objects visual representation. 
== Pong Game Design
#figure(
  image("figures/class_diagram_pong_on_engine.svg"),
  caption: [Pong class diagram using the game engine]
)<pong-class-diagram>
In #ref(<pong-class-diagram>) the classes derived from the game engine described in #ref(<game-engine-design>) are identified. To make pong, only one scene will be needed, and the context class will hold this at the only possible state. In pong, there are the two paddles, controlled by the players, and a single ball. These will be implemented as derived game objects with additional state variables, control signals and also extra logic to implement the features of the game (like moving the paddles up and down). For each derived game object, there is also a derived graphic shape class for a more clear separation. 

The ball object will be responsible for checking collision with either the boundary of the pong "field" and the paddles, hence the references to the paddles in the game. 
#pagebreak()
In #ref(<game-update-seq-diagram>), the game state update sequence is shown. Here, a call from the os to the context class is made to invoke the update. Following this invocation, the context, followed by the scene delegates the update and render methods to the individual game objects. Each state update consists of moving the paddles if the player activates their up or down buttons, and then moving the ball followed by a collision check. The following actions then clears the previous frames for each object and render new ones. 
#figure(
  image("figures/seq_game_update.svg"),
  caption: [Game update sequence diagram]
)<game-update-seq-diagram>

#pagebreak()
= Implementation

== GPU IP

#v(-20pt)
#figure(
  image("figures/parallel.svg", width: 100%),
  caption: [Command lists allow parallelization.],
) <parallel>

To parallize the graphics pipeline we propose a GPU IP capable of reading and parsing a _command list_ (CL) structure from memory. The CL acts much like a _ring buffer_ for a typical DMA, allowing to GPU to work independently. We decided on a stack like CL structure with a fixed size of 1024B. A single GPU command contains a 16-bit command type, and 112-bits of arguments. As the CPU writes commands it increments an index pointer, keeping track of where to insert the next command. A typical frame render is shown in #ref(<parallel>).

- *(a)* The CPU writes new commands and their arguments at the position of the index pointer.

- *(b)* The CPU informs the GPU of the FB and CL address and flips a control bit to start drawing. At this point the CPU is done with the graphics related task, and is free to do anything else.

- *(c)* The GPU burst reads the entire CL structure into an internal BRAM memory buffer, and starts parsing from index zero.

- *(d)* The GPU has read a `draw_rect` command with color `0x7FFF0000` i.e. draw a semi-transparent red rectangle. It starts by loading in the first row to be drawn into an internal BRAM row buffer.

- *(e)* The GPU draws part of the red rectangle to the associated row loaded into the row buffer. Since the rectangle is semi-transparent it performs hardware alpha compositing.

- *(f)* After blending the GPU burst writes the row back into the framebuffer.

#pagebreak()
== High-Level Synthesis of GPU

=== Pipelining
For performance critical loops we specify `#pragma HLS PIPELINE II=1`. This way the loop can process new inputs every single clock cycle. This is particularly important in our alpha blending loop.

=== AXI Burst
Getting an AXI master to burst using HLS can be tricky. As we don't have direct control over the hardware, we have to make sure that we satisfy some read/write requirements so the HLS compiler will make the AXI burst.

In HLS we define an internal `rowBuffer` with size $1920$. This way the AXI master interface can burst read up to an entire row of pixels from the frame buffer, perform blending if necessary, and then burst write the row back. The buffer will be implemented as BRAM. This technique is also used for reading the bound CL into an internal buffer.

#figure(
    block(
      fill: luma(255), 
      radius: 4pt,
      inset:	(x: 0pt, y: 3pt),
      outset: (y: 3pt),
      width: 100%,
      clip: false,
      [
        #align(left)[
        ```c
static ap_uint<32> rowBuffer[1920];
#pragma HLS RESOURCE variable=rowBuffer core=RAM_2P_BRAM

// Iterate over rows to be drawn

    READ_ROW: for (int col = 0; col < clipped_w; col++) {
    #pragma HLS PIPELINE II=1
        rowBuffer[col] = frameBuffer[row_offset + col];
    }

    // Perform blending in rowBuffer

    WRITE_ROW: for (int col = 0; col < clipped_w; col++) {
    #pragma HLS PIPELINE II=1
        frameBuffer[row_offset + col] = rowBuffer[col];
    }
        ```
        ]
      ]
    ),
    caption: [HLS implementation of burst read/write.],
    supplement: "Listing",
    kind: "code",
) <hls-burst>

=== Clipping
Clipping is the act of restricting the drawing area to a designated region, ensuring that only visible portions are rendered and displayed. This functionality acts both as a safety feature, denying writes to invalid regions of memory during runtime, but also proves useful for optimization.

We chose to implement clipping on the hardware level inside the GPU. By using the command: `SET_CLIP_CMD`, the clipping area for the GPU can be configured. When drawing a primitive its bounding box is compared with the clipping area as seen in #ref(<hls-clipping>).

#figure(
    block(
      fill: luma(255), 
      radius: 4pt,
      inset:	(x: 0pt, y: 3pt),
      outset: (y: 3pt),
      width: 100%,
      clip: false,
      [
        #align(left)[
        ```c
        int start_x = (x < clip_x) ? (int)clip_x : (int)x;
        int start_y = (y < clip_y) ? (int)clip_y : (int)y;
        int end_x = (x + w > clip_x + clip_w) ? (clip_x + clip_w) : (x + w);
        int end_y = (y + h > clip_y + clip_h) ? (clip_y + clip_h) : (y + h);
        ```
        ]
      ]
    ),
    caption: [HLS implementation of clipping.],
    supplement: "Listing",
    kind: "code",
) <hls-clipping>

#pagebreak()
== Game Engine

=== Bounding Box Redraw

#v(-10pt)
#figure(
  image("figures/scene_redraw.svg", width: 105%),
  caption: [Areas of the display have to redrawn completely when objects move.],
) <redraw>

A big optimization point is how we handle _stale areas_  in the framebuffers. When an object has moved it is not enough to simply redraw it, the previous area also has to be _cleared_. See in #ref(<redraw>) how stale areas occur.

- *(a)* The GPU draws two overlapping rectangles in the clear framebuffer `FB[0]`.

- *(b)* The active framebuffer have been switched. The GPU draws the rectangles in the clear framebuffer `FB[1]`.

- *(c)* After switching framebuffers `FB[1]` is now active again. `FB[1]` is stale since the objects from the first draw are still here. 

The easiest solution would be to simply redraw the entire scene every time. However, this takes far too long AND will easily violate our timing requirements, taking nearly 4 frames to just re-render the background. To correctly display moving graphics the game engine must be aware of these _stale areas_. To do this each game object has a memory of the two last positions. The game engine determines stale areas of the framebuffer and _clears_ them by rendering the background in just those areas. After clearing stale areas the engine renders the 
objects as usual.

== System

=== Cache Coherency
A surprising challenge during this project was _cache coherency_. A data cache (DCache) sits between the CPU and DDR. If the cache is not flushed then the command lists read by the GPU will be stale. In addition the cache must also be flushed to maintain framebuffer coherency between the GPU and the VDMA.

= Verification
For a HW/SW co-design such ass this continuous verification is absolutely necessary. In this chapter we will lightly detail our HLS testbench, how we verified the GPU IP on the Zynq FPGA, and how we verified correct render times.

== HLS Testbench
While developing the GPU we created a testbench that allocates a framebuffer and command list, adds draw commands, instantiates the GPU and finally "dumps" the framebuffer by saving the framebuffer as `.png`.

#figure(
    block(
      fill: luma(255), 
      radius: 4pt,
      inset:	(x: 0pt, y: 3pt),
      outset: (y: 3pt),
      width: 100%,
      clip: false,
      [
        #align(left)[
        ```c
int main()
{
    /* Initialize FB */
    fb_type* fb1 = allocate_fb(RESX, RESY, ARGB8888);

    /* Initialize CL */
    ap_uint<32> cl[256];
    for (int i = 0; i < 256; i++) {
        cl[i] = 0;
    }

    draw_rect(x, y, w, h, &cl)

    /* Call the GPU function */
    gpu(fb1->fb_array, status, cmd_fifo);

    /* Dump the rendered framebuffer using stb_image_write */
    save_fb_as_image(fb1, "framebuffer_image.png");
}
        ```
        ]
      ]
    ),
    caption: [HLS testbench for validating the GPU IP.],
    supplement: "Listing",
    kind: "code",
) <hls-burst>

== GPU Validation
#figure(
  image("figures/verification_fb_dump.png", width: 90%),
  caption: [Framebuffer dump from Zynq hardware design with custom GPU IP.],
) <verify_fb_dump>

After synthesizing the GPU, we added it to a simple block design in Vivado and wrote a verification application in the SDK. To verify the GPU we must test the three main capabilities:

1. Drawing of opaque rectangle.

2. Alpha blending.

3. Clipping.

A snippet of the application is shown in #ref(<gpu-validation>). We draw two overlapping rectangles, one red and one green. The green is translucent to verify the GPU correctly performs alpha blending. In addition a clipping rectangle is set to slightly smaller than than the frame, after which a 2000 pixel wide blue rectangle will be drawn. Since we at this point the video pipeline was not implemented, we memory dumped the framebuffer as a `.bin` file using the Xilinx SDK debugger. The binary file was converted to a `.png` using a Python script. See in #ref(<verify_fb_dump>) the framebuffer dump from the verification.

#figure(
    block(
      fill: luma(255), 
      radius: 4pt,
      inset:	(x: 0pt, y: 3pt),
      outset: (y: 3pt),
      width: 100%,
      clip: false,
      [
        #align(left)[
        ```c
/* Opaque red rectangle x, y = 100, w, h = 500 */
draw_rect(cl[frameIdx], 100, 100, 500, 500, 0xFFFF0000);
/* Translucent green rectangle x, y = 350, w, h = 500 */
draw_rect(cl[frameIdx], 350, 350, 500, 500, 0x7F00FF00);
/* Set clipping from x, y = 0 to x = 1500, y = 1080 */
set_clip(cl[frameIdx], 0, 0, 1500, 1080);
/* Out of bounds opaque blue rectangle x, y = 700, 800, w, h = 2000, 200 */
draw_rect(cl[frameIdx], 700, 800, 2000, 200, 0xFF0000FF);

/* Cache coherency */
Xil_DCacheFlushRange((UINTPTR)cl[frameIdx], 256)
GPU_BindFramebuffer(&FB);
GPU_BindCommandList(&cl[frameIdx]);
GPU_Start();
        ```
        ]
      ]
    ),
    caption: [SDK application for validating the GPU IP.],
    supplement: "Listing",
    kind: "code",
) <gpu-validation>

== RTOS
To verify the system performs as expected it was important the time spent in FreeRTOS idle, as well as the time it takes for the CPU to update the game state, and the GPU to render a frame. To accomplish this we initialize one of the PS timers, and set it to free running mode. We can then read the timer register before and after an action and calculate the time it took.

#pagebreak()
= Performance
To evaluate the performance of the system we are both interested in the raw rendering performance offered by the GPU, as well as the efficiency of our application loop for the Pong game.

The performance of our system when playing Pong is summarized in #ref(<performance>). The CPU only spends 0.08ms updating the game state and creating a new CL, leaving 16.5ms in idle until the next V-Sync. As for the GPU it takes on average 7.38ms to render a frame. This means that our application is extremely efficient, leaving a large amount of excess time for the CPU to increase the complexity of the game, or do other tasks entirely. In addition the GPU only uses 44% of the time available till the next V-Sync. All in all the platform should easily be capable of running more complex games or applications.

In #ref(<render-performance>) we compare the performance of the GPU versus the CPU when drawing a screen sized opaque rectangle, and when blending a screen sized rectangle with the background. We see impressively that the GPU can alpha blend just as fast as when simply writing in the framebuffer. For the CPU we see that it is extremely computationally heavy to do alpha blending, taking 6.5s (yes, seconds) to complete a single frame. 

#figure(table(
  columns: (0.5fr, 1fr),
  inset: 5pt,
  align: left,
  table.header([*Pong Performance*], [*Average Time*]),
  [`updateGame`], [0.08 ms],
  [`idle`], [16.5 ms],
  [GPU Render], [7.38 ms]
),
  caption: [
    Time spent running game logic vs. idle.
  ]
) <performance>

#figure(table(
  columns: (auto, 1fr, 1fr),
  inset: 5pt,
  align: (left, right, right),
  table.header([*Fullscreen Performance*], [*Average Time*], [*Pixels pr. ms*]),
  [CPU fullscreen Opaque], [1125.0 ms], [1843],
  [CPU Fullscreen Blending], [6504.0 ms], [318],
  [GPU fullscreen Opaque], [66.2 ms], [31323],
  [GPU Fullscreen Blending], [66.2 ms], [31323],
),
  caption: [
    GPU render performance.
  ]
) <render-performance>

#blockquote[
The CPU render times do not _really_ reflect real-world performance, as we draw using a simple loop i.e. a lot of time will be spent doing AXI transactions. Many ARM processors also support some form of SIMD which could also boost performance significantly. We still felt it had some importance as a baseline score though.
]