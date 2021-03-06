% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/router.R
\name{get_shortest_path}
\alias{get_shortest_path}
\title{Calculate the shortest path between two nodes on a graph}
\usage{
get_shortest_path(graphs, start_node, end_node)
}
\arguments{
\item{graphs}{\code{list} containing the two graphs and a map linking the two
to each other.}

\item{start_node}{Starting node for shortest path route.}

\item{end_node}{Ending node for shortest path route.}
}
\value{
\code{list} containing the \code{data.frame} of the graph elements
the shortest path lies on and the path distance.
}
\description{
Calculate the shortest path between two nodes on a graph
}
\examples{
\dontrun{
  graph <- road_data_sample
  start_pt <- c (11.603,48.163)
  end_pt <- c (11.608,48.167)
  pts <- select_vertices_by_coordinates (graph, start_pt, end_pt)
  route_start <- pts[1]
  route_end <- pts [2]
  get_shortest_path (graphs = graph, start_node = route_start,
  end_node = route_end)
}
}
