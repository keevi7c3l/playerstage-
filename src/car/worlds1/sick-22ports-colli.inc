
define sicklaser laser
(
  # laser-specific properties
  # factory settings for LMS200	
  range_max 2.0
  fov 90.0
  samples 360

  #samples 90 # still useful but much faster to compute

  # generic model properties
  color "blue"
  size [ 0.156 0.155 0.19 ] # dimensions from LMS200 data sheet	

  a0 ["infrDistSen" 0 10 0 7 ]
  a1 ["infrDistSen" 90 100 0 7 ]
  a2 ["tempSen" 0 360 0 0]
  d0 ["hallSen" 0 360 0 0.5]
  d1 ["colliSen" 130 140 0 0.4]
  d2 ["colliSen" 220 230 0 0.4]
  d3 ["colliSen" 175 185 0 0.4]

  temperaturesource [0 0 0]
)

define sicklaser2 laser
(
  # laser-specific properties
  # factory settings for LMS200	
  range_max 6.0
  fov 45.0
  samples 12

  #samples 90 # still useful but much faster to compute

  # generic model properties
  color "blue"
  size [ 0.156 0.155 0.19 ] # dimensions from LMS200 data sheet	
)

# extends sicklaser to add nice-looking but relatively expensive geometry
define fancysicklaser sicklaser
(
  # bottom
  block( 
    points 4
    point[0] [ -0.02 -0.077 ]
    point[1] [  0.078 -0.077 ]
    point[2] [  0.078  0.077 ]
    point[3] [ -0.02  0.077 ]
    z [0 0.02 ]
  )

  # back
  block(
    points 4
    point[0] [ -0.078 -0.077 ]
    point[1] [ -0.02  -0.077 ]
    point[2] [ -0.02   0.077 ]
    point[3] [ -0.078  0.077 ]
    z [0 0.21 ]
  )

  # top
  block( points 4
    point[0] [ -0.02 -0.077 ]
    point[1] [  0.078 -0.077 ]
    point[2] [  0.078  0.077 ]
    point[3] [ -0.02  0.077 ]
    z [0.12 0.21 ]  
  )

  # laser bit
  block( points 4
    point[0] [ -0.02 -0.05 ]
	 point[1] [  0.06 -0.05 ]
	 point[2] [  0.06  0.05 ]
	 point[3] [ -0.02  0.05 ]
	 z [0.02 0.12 ]  
   color "gray10"
  )
)



